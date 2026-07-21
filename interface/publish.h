#pragma once
#include <nlohmann/json.hpp>
#include <string>

namespace card_batch {

struct PublishResult {
    int total_cards = 0;     // 源文件里的总卡数
    int published   = 0;     // 实际发布的（approved）
    int skipped     = 0;     // 跳过的（非 approved）
    std::string output_path; // 写到哪里了
};

// 从 input_path 读，筛 approved 的卡，写到 output_path
// 原子写（.tmp + rename）
PublishResult publish(const std::string& input_path,
                     const std::string& output_path);

}  // namespace card_batch