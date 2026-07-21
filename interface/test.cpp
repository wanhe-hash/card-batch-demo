#include "provider.h"
#include "knowledge_points.h"
#include "batch_meta.h"
#include "retry.h"
#include "checkpoint.h"
#include "review.h"
#include "publish.h"

// === 整合所有模块 ===
// provider:     LLM API 调用
// knowledge_points: 知识点数据
// batch_meta:   批次元信息
// retry:        指数退避重试
// checkpoint:   断点续跑
// review:       审核状态管理
// publish:      发布过滤输出

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <cstdlib>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>

namespace card_batch {

// ============================================================
// 数据结构
// ============================================================
struct Pair {
    int id;
    std::string term;
    std::string definition;
    Review review;                       // 默认 pending
};

struct CardGroup {
    int group_id;
    std::string topic;
    std::vector<Pair> pairs;
};

// ============================================================
// 工具函数
// ============================================================

bool parseOuterWithRepair(const std::string& resp, std::string& content) {
    const std::string marker = "\"content\":\"";
    auto pos = resp.find(marker);
    if (pos == std::string::npos) {
        std::cerr << "  [错误] 找不到 content 字段\n";
        return false;
    }
    pos += marker.size();

    std::string escaped;
    bool escape = false;
    for (size_t i = pos; i < resp.size(); ++i) {
        char c = resp[i];
        if (escape)        { escaped += c; escape = false; }
        else if (c == '\\'){ escaped += c; escape = true;  }
        else if (c == '"') { break;                       }
        else               { escaped += c;                 }
    }

    try {
        std::string wrapped = "\"" + escaped + "\"";
        content = nlohmann::json::parse(wrapped).get<std::string>();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "  unescape 失败: " << e.what() << "\n";
        std::cerr << "  escaped: " << escaped << "\n";
        return false;
    }
}

std::string cleanJson(const std::string& raw) {
    std::string s = raw;
    auto first = s.find("```");
    if (first == std::string::npos) return s;
    s = s.substr(first);
    auto last = s.rfind("```");
    if (last != std::string::npos && last > 0) s = s.substr(0, last);
    auto nl = s.find('\n');
    if (nl != std::string::npos) s = s.substr(nl + 1);
    return s;
}

std::string buildPrompt(const KnowledgePoint& kp) {
    return std::string(
        "你是一个证券从业资格考试的出题老师。\n"
        "请围绕下面这个知识点，生成 5 对配对卡片（用于连连看式配对游戏）。\n"
        "要求：\n"
        "1. 每对卡片由 term（术语）和 definition（定义）组成\n"
        "2. 术语和定义必须严格对应，不能有歧义\n"
        "3. 5 对术语之间不能重复\n"
        "4. 严格按下面这个 JSON 对象输出，禁止 markdown 代码块，禁止任何解释文字：\n"
        "{\"pairs\":["
            "{\"id\":1,\"term\":\"...\",\"definition\":\"...\"},"
            "{\"id\":2,\"term\":\"...\",\"definition\":\"...\"},"
            "{\"id\":3,\"term\":\"...\",\"definition\":\"...\"},"
            "{\"id\":4,\"term\":\"...\",\"definition\":\"...\"},"
            "{\"id\":5,\"term\":\"...\",\"definition\":\"...\"}"
        "]}\n"
        "\n"
        "知识点：") + kp.topic + "\n" +
        "背景：" + kp.description;
}

bool generateGroup(const ChatConfig& cfg,
                   const KnowledgePoint& kp,
                   CardGroup& group) {
    std::string resp = chat(cfg, buildPrompt(kp));
    if (resp.empty()) return false;

    try {
        std::string content;
        if (!parseOuterWithRepair(resp, content)) {
            std::cerr << "  raw: " << resp << "\n";
            return false;
        }
        content = cleanJson(content);
        auto j = nlohmann::json::parse(content);

        group.group_id = kp.id;
        group.topic    = kp.topic;
        group.pairs.clear();
        for (const auto& p : j["pairs"]) {
            Pair pair;
            pair.id         = p.value("id", 0);
            pair.term       = p.value("term", "");
            pair.definition = p.value("definition", "");
            // pair.review 默认 pending
            group.pairs.push_back(pair);
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "  parse error: " << e.what() << "\n";
        std::cerr << "  raw: " << resp << "\n";
        return false;
    }
}

std::string tryGenerateGroup(const ChatConfig& cfg,
                              const KnowledgePoint& kp,
                              CardGroup& group) {
    return generateGroup(cfg, kp, group) ? "ok" : "";
}

}  // namespace card_batch

// ============================================================
// CLI 参数解析
// ============================================================
namespace {

struct Args {
    std::string cmd;
    int group_id  = -1;
    int pair_id   = -1;
    std::string status;
    std::string reviewer;
    std::string notes;
    std::string list_status;
    std::string output_path = "published.json";
};

Args parseArgs(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];
        if      (s == "review")                a.cmd = "review";
        else if (s == "publish")               a.cmd = "publish";
        else if (s == "-g"  && i + 1 < argc)    a.group_id  = std::atoi(argv[++i]);
        else if (s == "-p"  && i + 1 < argc)    a.pair_id   = std::atoi(argv[++i]);
        else if (s == "-s"  && i + 1 < argc)    a.status    = argv[++i];
        else if (s == "-r"  && i + 1 < argc)    a.reviewer  = argv[++i];
        else if (s == "-n"  && i + 1 < argc)    a.notes     = argv[++i];
        else if (s == "--list" && i + 1 < argc) a.list_status = argv[++i];
        else if (s == "--output" && i + 1 < argc) a.output_path = argv[++i];
    }
    return a;
}

}  // namespace  ←←← 关键！匿名 namespace 必须在 main 之前关掉

// ============================================================
// 主入口
// ============================================================
int main(int argc, char** argv) {
    Args args = parseArgs(argc, argv);

    // ===== publish 子命令 =====
    if (args.cmd == "publish") {
        auto r = card_batch::publish("cards.json", args.output_path);
        if (r.total_cards == 0) {
            std::cerr << "publish 失败：cards.json 不存在或损坏\n";
            return 1;
        }
        std::cout << "✓ 发布完成\n";
        std::cout << "  源卡数:   " << r.total_cards << "\n";
        std::cout << "  已发布:   " << r.published   << " 张 (approved)\n";
        std::cout << "  跳过:     " << r.skipped     << " 张 (pending/rejected/needs_revision)\n";
        std::cout << "  输出到:   " << r.output_path << "\n";
        return 0;
    }

    // ===== review 子命令 =====
    if (args.cmd == "review") {
        if (!args.list_status.empty()) {
            auto items = card_batch::listByStatus("cards.json", args.list_status);
            std::cout << "状态 [" << args.list_status << "] 共 "
                      << items.size() << " 张：\n";
            for (const auto& it : items) {
                std::cout << "  [G" << it["group_id"].get<int>()
                          << "-P" << it["pair_id"].get<int>() << "] "
                          << it["term"].get<std::string>()
                          << " | " << it["reviewer"].get<std::string>()
                          << ": " << it["notes"].get<std::string>() << "\n";
            }
            return 0;
        }

        if (args.group_id < 0 || args.pair_id < 0) {
            std::cerr << "用法：\n";
            std::cerr << "  ./chat_demo review -g <group> -p <pair> "
                      << "-s <status> [-r reviewer] [-n notes]\n";
            std::cerr << "  ./chat_demo review --list <status>\n";
            std::cerr << "  status 可选: pending / approved / rejected / needs_revision\n";
            return 1;
        }

        card_batch::Review r;
        r.status   = args.status.empty() ? "pending" : args.status;
        r.reviewer = args.reviewer;
        r.notes    = args.notes;
        if (card_batch::updateReview("cards.json", args.group_id, args.pair_id, r)) {
            std::cout << "✓ G" << args.group_id << "-P" << args.pair_id
                      << " 状态更新为 [" << r.status << "]\n";
            return 0;
        }
        return 1;
    }

    // ===== 正常生成流程 =====
    curl_global_init(CURL_GLOBAL_DEFAULT);

    card_batch::ChatConfig cfg{
        .api_key   = "",
        .base_url  = "https://api.deepseek.com/v1",
        .model     = "deepseek-chat",
        .json_mode = true
    };

    if (const char* k = std::getenv("DEEPSEEK_API_KEY")) {
        cfg.api_key = k;
    } else {
        std::cerr << "错误：未设置环境变量 DEEPSEEK_API_KEY\n";
        std::cerr << "      先执行：export DEEPSEEK_API_KEY=\"sk-你的key\"\n";
        return 1;
    }

    auto meta = card_batch::makeMeta(cfg.model, cfg.base_url);
    std::cout << "Batch ID: " << meta.batch_id << "\n\n";

    // === 加载已有 checkpoint ===
    auto state = card_batch::loadCheckpoint("cards.json");

    if (std::getenv("FRESH")) {
        std::cout << "[FRESH] 强制重新跑，丢弃已有进度\n\n";
        state.groups = nlohmann::json::array();
        state.done.clear();
    } else if (!state.done.empty()) {
        std::cout << "[Resume] cards.json 已有 " << state.done.size()
                  << " 组，继续未完成的\n";
        std::cout << "         已完成 IDs: ";
        for (int id : state.done) std::cout << id << " ";
        std::cout << "\n";
        if (!state.batch_id.empty()) {
            meta.batch_id = state.batch_id;
            std::cout << "         沿用原 batch_id: " << meta.batch_id << "\n";
        }
    }
    std::cout << "\n";

    nlohmann::json groups = state.groups;
    int newly_failed = 0;
    card_batch::RetryConfig retry_cfg;

    for (const auto& kp : card_batch::all()) {
        if (state.done.count(kp.id)) {
            std::cout << "=== [" << kp.id << "] " << kp.topic
                      << " === [已存在，跳过]\n\n";
            continue;
        }

        std::cout << "=== [" << kp.id << "] " << kp.topic << " ===\n";

        card_batch::CardGroup group;
        std::string result = card_batch::withRetry(retry_cfg, [&] {
            return card_batch::tryGenerateGroup(cfg, kp, group);
        });

        if (result.empty()) {
            std::cerr << "  (跳过)\n\n";
            newly_failed++;
            continue;
        }

        nlohmann::json pairs_json = nlohmann::json::array();
        for (const auto& p : group.pairs) {
            std::cout << "  [" << p.id << "] " << p.term
                      << "  ⇄  " << p.definition << "\n";
            pairs_json.push_back({
                {"id", p.id},
                {"term", p.term},
                {"definition", p.definition},
                {"review", card_batch::reviewToJson(p.review)}
            });
        }
        std::cout << "\n";

        groups.push_back({
            {"group_id", kp.id},
            {"topic",    kp.topic},
            {"pairs",    pairs_json}
        });

        if (!card_batch::saveCheckpoint("cards.json", meta, groups, newly_failed)) {
            std::cerr << "  [警告] checkpoint 保存失败\n";
        } else {
            std::cout << "  [checkpoint] 已保存 (" << groups.size() << " 组)\n\n";
        }
    }

    int total_pairs = 0;
    for (const auto& g : groups) {
        total_pairs += g.value("pairs", nlohmann::json::array()).size();
    }
    meta.total_groups  = groups.size();
    meta.total_pairs   = total_pairs;
    meta.failed_groups = newly_failed;
    card_batch::saveCheckpoint("cards.json", meta, groups, newly_failed);

    std::cout << "===== 完成 =====\n";
    std::cout << "Batch ID      : " << meta.batch_id << "\n";
    std::cout << "Model         : " << meta.model    << "\n";
    std::cout << "生成组数       : " << groups.size()  << " / 10\n";
    std::cout << "生成卡片       : " << total_pairs   << " 张\n";
    std::cout << "失败知识点     : " << newly_failed  << " 个\n";

    curl_global_cleanup();
    return (groups.size() == 10 && newly_failed == 0) ? 0 : 1;
}