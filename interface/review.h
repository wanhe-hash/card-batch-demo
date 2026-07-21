#pragma once
#include <nlohmann/json.hpp>
#include <string>

namespace card_batch {

struct Review {
    std::string status      = "pending";  // pending / approved / rejected / needs_revision
    std::string reviewed_at = "";         // ISO 8601，自动填
    std::string reviewer    = "";
    std::string notes       = "";
};

// 从 JSON 读 review（缺失字段用默认值，老数据兼容）
Review reviewFromJson(const nlohmann::json& j);

// 把 review 写成 JSON
nlohmann::json reviewToJson(const Review& r);

// 更新 cards.json 里指定 group/pair 的 review 状态
// 找不到对应卡片返回 false；cards.json 不存在也返 false
bool updateReview(const std::string& path,
                  int group_id,
                  int pair_id,
                  const Review& new_review);

// 列出某状态的卡片（CLI --list 用）
nlohmann::json listByStatus(const std::string& path,
                             const std::string& status);

}  // namespace card_batch