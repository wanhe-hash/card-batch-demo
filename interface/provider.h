#pragma once
#include <string>
#include <iostream>
#include <chrono>
#include <thread>
#include <random>

namespace card_batch {

struct ChatConfig {
    std::string api_key;
    std::string base_url;
    std::string model;
    bool json_mode = false;
};

std::string chat(const ChatConfig& cfg, const std::string& prompt);

}  // namespace card_batch