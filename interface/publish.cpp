#include "publish.h"
#include "review.h"

#include <cstdio>
#include <fstream>
#include <iostream>

namespace card_batch {

namespace {

bool isPublishedStatus(const std::string& s) {
    return s == "approved";
}

std::string nowIso8601() {
    auto t = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
    std::tm tm_buf{};
    localtime_r(&t, &tm_buf);
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

}  // namespace

PublishResult publish(const std::string& input_path,
                     const std::string& output_path) {
    PublishResult result;
    result.output_path = output_path;

    // 1) 读源
    nlohmann::json j;
    {
        std::ifstream in(input_path);
        if (!in) {
            std::cerr << "  [publish] 无法打开 " << input_path << "\n";
            return result;
        }
        try {
            in >> j;
        } catch (const std::exception& e) {
            std::cerr << "  [publish] 解析 " << input_path
                      << " 失败: " << e.what() << "\n";
            return result;
        }
    }

    if (!j.contains("groups") || !j["groups"].is_array()) {
        std::cerr << "  [publish] " << input_path
                  << " 没有 groups 数组\n";
        return result;
    }

    // 2) 过滤
    nlohmann::json published_groups = nlohmann::json::array();
    int total = 0, pub = 0, skip = 0;

    for (const auto& g : j["groups"]) {
        int gid = g.value("group_id", -1);
        std::string topic = g.value("topic", "");

        nlohmann::json approved_pairs = nlohmann::json::array();
        if (g.contains("pairs") && g["pairs"].is_array()) {
            for (const auto& p : g["pairs"]) {
                total++;
                std::string status = p.value("review", nlohmann::json::object())
                                       .value("status", "pending");
                if (!isPublishedStatus(status)) {
                    skip++;
                    continue;
                }
                // 不带 review 块（游戏不需要）
                approved_pairs.push_back({
                    {"id", p.value("id", -1)},
                    {"term", p.value("term", "")},
                    {"definition", p.value("definition", "")}
                });
                pub++;
            }
        }

        // 即使这一组全被拒，也保留 group 框架（topic 在）
        published_groups.push_back({
            {"group_id", gid},
            {"topic", topic},
            {"pairs", approved_pairs}
        });
    }

    // 3) 构造输出
    nlohmann::json output = {
        {"meta", {
            {"source_batch_id", j.value("meta", nlohmann::json::object())
                                .value("batch_id", "")},
            {"source_model", j.value("meta", nlohmann::json::object())
                                .value("model", "")},
            {"published_at", nowIso8601()},
            {"total_in_source", total},
            {"published", pub},
            {"skipped", skip}
        }},
        {"groups", published_groups}
    };

    // 4) 原子写
    const std::string tmp = output_path + ".tmp";
    {
        std::ofstream out(tmp);
        if (!out) {
            std::cerr << "  [publish] 写失败\n";
            return result;
        }
        out << output.dump(2);
    }
    if (std::rename(tmp.c_str(), output_path.c_str()) != 0) {
        std::cerr << "  [publish] rename 失败\n";
        return result;
    }

    result.total_cards = total;
    result.published   = pub;
    result.skipped     = skip;
    return result;
}

}  // namespace card_batch