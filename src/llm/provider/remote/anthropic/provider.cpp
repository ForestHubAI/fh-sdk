// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include <algorithm>
#include <utility>

#include "foresthub/llm/provider/remote/anthropic.hpp"
#include "mapping.hpp"

namespace foresthub {
namespace provider {
namespace remote {

using json = nlohmann::json;

static const char* const kDefaultBaseUrl = "https://api.anthropic.com";
static const int kDefaultMaxTokens = 4096;

AnthropicProvider::AnthropicProvider(const config::ProviderConfig& cfg, std::shared_ptr<llm::HttpClient> http_client)
    : http_(std::move(http_client)),
      api_key_(cfg.api_key),
      base_url_(cfg.base_url.empty() ? kDefaultBaseUrl : cfg.base_url),
      supported_models_(cfg.supported_models.begin(), cfg.supported_models.end()) {
    // Strip trailing slash to avoid double slashes when appending paths.
    if (!base_url_.empty() && base_url_.back() == '/') {
        base_url_.pop_back();
    }
    cached_headers_ = {
        {"Content-Type", "application/json"}, {"x-api-key", api_key_}, {"anthropic-version", "2023-06-01"}};
}

llm::ProviderID AnthropicProvider::ProviderId() const {
    return "anthropic";
}

bool AnthropicProvider::SupportsModel(const llm::ModelID& model) const {
    // Empty list = accept all models
    if (supported_models_.empty()) {
        return true;
    }
    return std::find(supported_models_.begin(), supported_models_.end(), model) != supported_models_.end();
}

std::string AnthropicProvider::Health() const {
    std::string url = base_url_ + "/v1/models";
    llm::HttpResponse resp = http_->Get(url, cached_headers_);
    if (resp.status_code >= 200 && resp.status_code < 300) {
        return "";
    }
    return "health check failed, status: " + std::to_string(resp.status_code);
}

std::shared_ptr<llm::ChatResponse> AnthropicProvider::Chat(const llm::ChatRequest& req) {
    std::string url = base_url_ + "/v1/messages";

    json j_req = ToAnthropicRequest(req, kDefaultMaxTokens);
    std::string body = j_req.dump();

    // Retry with linear backoff (500ms * attempt)
    llm::HttpResponse resp;
    unsigned long attempts = 0;
    const unsigned long max_attempts = 2;

    while (attempts < max_attempts) {
        if (attempts > 0) {
            http_->Delay(500 * attempts);
        }

        resp = http_->Post(url, cached_headers_, body);

        if (resp.status_code >= 200 && resp.status_code < 300) {
            break;
        }

        // 4xx: don't retry except for rate limits (429) and timeouts (408)
        if (resp.status_code >= 400 && resp.status_code < 500) {
            if (resp.status_code != 408 && resp.status_code != 429) {
                break;
            }
        }

        attempts++;
    }

    if (resp.status_code < 200 || resp.status_code >= 300) {
        return nullptr;
    }

    json j_resp = json::parse(resp.body, nullptr, false);
    if (j_resp.is_discarded() || !j_resp.is_object()) {
        return nullptr;
    }

    return FromAnthropicResponse(j_resp);
}

}  // namespace remote
}  // namespace provider
}  // namespace foresthub
