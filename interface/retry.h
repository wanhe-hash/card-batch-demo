// retry.h
#pragma once
#include <string>
#include <functional>
namespace card_batch {

struct RetryConfig {
    int max_attempts       = 3;     // 总尝试次数（含首次）
    int initial_delay_ms   = 1000;  // 第 1 次重试前等多久
    double backoff_mult    = 2.0;   // 每次重试间隔 × 这个倍数
    int max_delay_ms       = 10000; // 间隔上限
    int jitter_ms          = 500;   // 随机抖动上限
};

// 包装 fn，最多尝试 max_attempts 次
// 返回最后一次 fn 的结果（空 = 全失败）
// 重试过程会把信息打到 stderr
std::string withRetry(const RetryConfig& rc,
                      std::function<std::string()> fn);

}  // namespace card_batch