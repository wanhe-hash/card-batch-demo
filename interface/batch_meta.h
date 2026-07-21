#pragma once
#include <string>

namespace card_batch {

struct BatchMeta {
    std::string batch_id;
    std::string model;
    std::string base_url;
    std::string generated_at;
    int total_groups  = 0;
    int total_pairs   = 0;
    int failed_groups = 0;
};

std::string newBatchId();
BatchMeta makeMeta(const std::string& model,
                   const std::string& base_url);

}  // namespace card_batch