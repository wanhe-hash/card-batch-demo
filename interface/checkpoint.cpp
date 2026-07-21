#include "checkpoint.h"
#include "batch_meta.h"

#include <cstdio>     // std::rename
#include <fstream>
#include <iostream>

namespace card_batch {

CheckpointState loadCheckpoint(const std::string& path) {
    CheckpointState state;

    std::ifstream in(path);
    if (!in) {
        return state;  // 文件不存在
    }

    try {
        nlohmann::json j;
        in >> j;

        if (j.contains("groups") && j["groups"].is_array()) {
            state.groups = j["groups"];
            for (const auto& g : state.groups) {
                state.done.insert(g.value("group_id", -1));
            }
        }

        if (j.contains("meta") && j["meta"].is_object()) {
            const auto& m = j["meta"];
            state.batch_id     = m.value("batch_id", "");
            state.model        = m.value("model", "");
            state.base_url     = m.value("base_url", "");
            state.generated_at = m.value("generated_at", "");
        }
    } catch (const std::exception& e) {
        std::cerr << "  [checkpoint] 解析 " << path
                  << " 失败：" << e.what()
                  << "（将当作空状态）\n";
    }

    return state;
}

bool saveCheckpoint(const std::string& path,
                    const BatchMeta& meta,
                    const nlohmann::json& groups,
                    int failed_groups) {
    int total_pairs = 0;
    for (const auto& g : groups) {
        total_pairs += g.value("pairs", nlohmann::json::array()).size();
    }

    nlohmann::json output = {
        {"meta", {
            {"batch_id",     meta.batch_id},
            {"model",        meta.model},
            {"base_url",     meta.base_url},
            {"generated_at", meta.generated_at},
            {"total_groups", static_cast<int>(groups.size())},
            {"total_pairs",  total_pairs},
            {"failed_groups",failed_groups}
        }},
        {"groups", groups}
    };

    // 原子写：先 .tmp 再 rename
    const std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp);
        if (!out) {
            std::cerr << "  [checkpoint] 写 " << tmp << " 失败\n";
            return false;
        }
        out << output.dump(2);
    }

    if (std::rename(tmp.c_str(), path.c_str()) != 0) {
        std::cerr << "  [checkpoint] rename 失败\n";
        return false;
    }
    return true;
}

}  // namespace card_batch