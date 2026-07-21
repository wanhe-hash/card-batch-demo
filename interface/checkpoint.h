#pragma once
#include <nlohmann/json.hpp>
#include <set>
#include <string>

namespace card_batch {

struct BatchMeta;  // 定义在 batch_meta.h，前向声明

struct CheckpointState {
    nlohmann::json groups = nlohmann::json::array();
    std::set<int> done;          // 已完成的 group_id
    std::string batch_id;        // 文件里的原 batch_id
    std::string model;
    std::string base_url;
    std::string generated_at;
};

// 从 path 加载已有的 checkpoint
// 文件不存在 / 损坏 → 返回"全空"的 state（不抛异常）
CheckpointState loadCheckpoint(const std::string& path);

// 原子写入：先写 .tmp，再 rename
// failed_groups 是本次跑新增失败的数
bool saveCheckpoint(const std::string& path,
                    const BatchMeta& meta,
                    const nlohmann::json& groups,
                    int failed_groups);

}  // namespace card_batch