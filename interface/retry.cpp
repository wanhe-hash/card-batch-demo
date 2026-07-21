#include "retry.h"
#include <functional>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>

namespace card_batch {

namespace {

// 计算"第 retry_attempt 次重试前"要等多少毫秒
// retry_attempt = 1 → initial_delay_ms
// retry_attempt = 2 → initial_delay_ms * backoff_mult
// retry_attempt = N → initial_delay_ms * backoff_mult^(N-1)
int computeDelayMs(int retry_attempt, const RetryConfig& rc) {
    double delay = static_cast<double>(rc.initial_delay_ms);
    for (int i = 1; i < retry_attempt; ++i) {
        delay *= rc.backoff_mult;
    }
    if (delay > rc.max_delay_ms) {
        delay = rc.max_delay_ms;
    }

    // 随机抖动
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> jitter(0, rc.jitter_ms);
    return static_cast<int>(delay) + jitter(rng);
}

}  // namespace

std::string withRetry(const RetryConfig& rc,
                      std::function<std::string()> fn) {
    std::string last;

    for (int attempt = 1; attempt <= rc.max_attempts; ++attempt) {
        last = fn();

        if (!last.empty()) {
            if (attempt > 1) {
                std::cerr << "  [重试] 第 " << attempt
                          << " 次成功（共试 " << attempt << " 次）\n";
            }
            return last;
        }

        // 失败：还有重试机会就等
        if (attempt < rc.max_attempts) {
            int delay_ms = computeDelayMs(attempt, rc);
            std::cerr << "  [重试] 第 " << attempt << " 次失败，"
                      << delay_ms << "ms 后重试 ("
                      << attempt + 1 << "/" << rc.max_attempts << ")\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
    }

    std::cerr << "  [重试] " << rc.max_attempts
              << " 次全部失败，放弃\n";
    return last;
}

}  // namespace card_batch