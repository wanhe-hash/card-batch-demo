#include "provider.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>

namespace card_batch {

namespace {

// 接住 libcurl 的响应体（仅本文件可见）
size_t WriteCb(void* contents, size_t size, size_t nmemb, std::string* out) {
    out->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

}  // namespace

std::string chat(const ChatConfig& cfg, const std::string& prompt) {
    CURL* curl = curl_easy_init();
    if (!curl) return {};

    nlohmann::json req = {
        {"model", cfg.model},
        {"messages", nlohmann::json::array({
            {{"role", "user"}, {"content", prompt}}
        })},
        {"temperature", 0.7}
    };

    if (cfg.json_mode) {
        req["response_format"] = {{"type", "json_object"}};
    }

    std::string body = req.dump();
    const std::string url = cfg.base_url + "/chat/completions";

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    const std::string auth = "Authorization: Bearer " + cfg.api_key;
    headers = curl_slist_append(headers, auth.c_str());

    std::string resp;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode rc = curl_easy_perform(curl);
    long http_code = 0;
    if (rc == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) {
        std::cerr << "curl error: " << curl_easy_strerror(rc) << "\n";
        return {};
    }
    if (http_code >= 400) {
        std::cerr << "http " << http_code << ": " << resp << "\n";
        return {};
    }
    return resp;
}

}  // namespace card_batch