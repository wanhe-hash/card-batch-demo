#include "review.h"

#include <chrono>
#include <ctime>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>

namespace card_batch {

namespace {

std::string nowIso8601() {
    auto t = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
    std::tm tm_buf{};
    localtime_r(&t, &tm_buf);
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

bool isValidStatus(const std::string& s) {
    static const std::set<std::string> valid = {
        "pending", "approved", "rejected", "needs_revision"
    };
    return valid.count(s) > 0;
}

}  // namespace

Review reviewFromJson(const nlohmann::json& j) {
    Review r;
    r.status      = j.value("status", "pending");
    r.reviewed_at = j.value("reviewed_at", "");
    r.reviewer    = j.value("reviewer", "");
    r.notes       = j.value("notes", "");
    return r;
}

nlohmann::json reviewToJson(const Review& r) {
    return {
        {"status",      r.status},
        {"reviewed_at", r.reviewed_at},
        {"reviewer",    r.reviewer},
        {"notes",       r.notes}
    };
}

bool updateReview(const std::string& path,
                  int group_id,
                  int pair_id,
                  const Review& new_review) {
    if (!isValidStatus(new_review.status)) {
        std::cerr << "  [review] 非法 status: " << new_review.status
                  << "（允许: pending/approved/rejected/needs_revision）\n";
        return false;
    }

    nlohmann::json j;
    {
        std::ifstream in(path);
        if (!in) {
            std::cerr << "  [review] 无法打开 " << path << "\n";
            return false;
        }
        in >> j;
    }

    if (!j.contains("groups") || !j["groups"].is_array()) {
        std::cerr << "  [review] " << path << " 没有 groups 数组\n";
        return false;
    }

    bool found = false;
    for (auto& g : j["groups"]) {
        if (g.value("group_id", -1) != group_id) continue;
        if (!g.contains("pairs") || !g["pairs"].is_array()) continue;
        for (auto& p : g["pairs"]) {
            if (p.value("id", -1) != pair_id) continue;

            Review r = new_review;
            if (r.reviewed_at.empty()) {
                r.reviewed_at = nowIso8601();   // 自动填时间
            }
            p["review"] = reviewToJson(r);
            found = true;
            break;
        }
        if (found) break;
    }

    if (!found) {
        std::cerr << "  [review] 找不到 group=" << group_id
                  << " pair=" << pair_id << "\n";
        return false;
    }

    // 原子写
    const std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp);
        if (!out) {
            std::cerr << "  [review] 写失败\n";
            return false;
        }
        out << j.dump(2);
    }
    if (std::rename(tmp.c_str(), path.c_str()) != 0) {
        std::cerr << "  [review] rename 失败\n";
        return false;
    }
    return true;
}

nlohmann::json listByStatus(const std::string& path,
                             const std::string& status) {
    nlohmann::json result = nlohmann::json::array();

    std::ifstream in(path);
    if (!in) return result;

    nlohmann::json j;
    try { in >> j; } catch (...) { return result; }

    if (!j.contains("groups") || !j["groups"].is_array()) return result;

    for (const auto& g : j["groups"]) {
        int gid = g.value("group_id", -1);
        std::string topic = g.value("topic", "");
        if (!g.contains("pairs") || !g["pairs"].is_array()) continue;

        for (const auto& p : g["pairs"]) {
            std::string s = p.value("review", nlohmann::json::object())
                              .value("status", "pending");
            if (s != status) continue;
            result.push_back({
                {"group_id", gid},
                {"pair_id", p.value("id", -1)},
                {"topic", topic},
                {"term", p.value("term", "")},
                {"definition", p.value("definition", "")},
                {"reviewer", p.value("review", nlohmann::json::object())
                              .value("reviewer", "")},
                {"notes", p.value("review", nlohmann::json::object())
                              .value("notes", "")}
            });
        }
    }
    return result;
}

}  // namespace card_batch