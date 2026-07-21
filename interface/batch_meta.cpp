#include "batch_meta.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace card_batch {

std::string newBatchId() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) % 1000;

    std::tm tm_buf{};
    localtime_r(&t, &tm_buf);

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y%m%d-%H%M%S")
        << "-" << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

BatchMeta makeMeta(const std::string& model,
                   const std::string& base_url) {
    BatchMeta m;
    m.batch_id = newBatchId();
    m.model    = model;
    m.base_url = base_url;

    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
    localtime_r(&t, &tm_buf);

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S");
    m.generated_at = oss.str();

    return m;
}

}  // namespace card_batch