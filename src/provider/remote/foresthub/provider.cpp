#include <algorithm>
#include <utility>

#include "foresthub/core/serialization.hpp"
#include "foresthub/provider/remote/foresthub.hpp"

namespace foresthub {
namespace provider {
namespace remote {

using json = nlohmann::json;

ForestHubProvider::ForestHubProvider(const config::ProviderConfig& cfg, std::shared_ptr<core::HttpClient> http_client)
    : http_(std::move(http_client)),
      api_key_(cfg.api_key),
      base_url_(cfg.base_url),
      supported_models_(cfg.supported_models) {
    // Strip trailing slash to avoid double slashes when appending paths.
    if (!base_url_.empty() && base_url_.back() == '/') {
        base_url_.pop_back();
    }
    cached_headers_ = {{"Content-Type", "application/json"}, {"Device-Key", api_key_}, {"Accept", "application/json"}};
}

core::ProviderID ForestHubProvider::ProviderId() const {
    return "forest-hub";
}

std::string ForestHubProvider::Health() const {
    std::string url = base_url_ + "/llm/health";
    core::HttpResponse resp = http_->Get(url, cached_headers_);
    if (resp.status_code >= 200 && resp.status_code < 300) {
        return "";
    }
    return "health check failed, status: " + std::to_string(resp.status_code);
}

bool ForestHubProvider::SupportsModel(const core::ModelID& model) const {
    if (supported_models_.empty()) {
        return false;
    }

    return std::find(supported_models_.begin(), supported_models_.end(), model) != supported_models_.end();
}

std::shared_ptr<core::ChatResponse> ForestHubProvider::Chat(const core::ChatRequest& req) {
    std::string url = base_url_ + "/llm/generate";

    json j_req = req;
    std::string body = j_req.dump();

    // Retry with linear backoff (500ms * attempt)
    core::HttpResponse resp;
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

    auto chat_resp = std::make_shared<core::ChatResponse>();
    *chat_resp = j_resp.get<core::ChatResponse>();

    return chat_resp;
}

}  // namespace remote
}  // namespace provider
}  // namespace foresthub
