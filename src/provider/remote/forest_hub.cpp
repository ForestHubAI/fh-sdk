#include "foresthub/provider/remote/forest_hub.hpp"

#include <algorithm>
#include <utility>

#include "foresthub/core/serialization.hpp"

namespace foresthub {
namespace provider {
namespace remote {

using json = foresthub::core::json;

ForestHubProvider::ForestHubProvider(const config::RemoteConfig& cfg, std::shared_ptr<core::HttpClient> http_client)
    : http(std::move(http_client)),
      base_url(cfg.base_url),
      api_key(cfg.api_key),
      supported_models(cfg.supported_models) {
    // Strip trailing slash to avoid double slashes when appending paths.
    if (!base_url.empty() && base_url.back() == '/') {
        base_url.pop_back();
    }
    cached_headers_ = {{"Content-Type", "application/json"}, {"Device-Key", api_key}, {"Accept", "application/json"}};
}

core::ProviderID ForestHubProvider::ProviderId() const {
    return "forest-hub";
}

std::string ForestHubProvider::Health() const {
    std::string url = base_url + "/llm/health";
    core::HttpResponse resp = http->Get(url, cached_headers_);
    if (resp.status_code >= 200 && resp.status_code < 300) {
        return "";
    }
    return "health check failed, status: " + std::to_string(resp.status_code);
}

bool ForestHubProvider::SupportsModel(const core::ModelID& model) const {
    if (supported_models.empty()) {
        return false;
    }

    return std::find(supported_models.begin(), supported_models.end(), model) != supported_models.end();
}

std::shared_ptr<core::ChatResponse> ForestHubProvider::Chat(const core::ChatRequest& req) {
    std::string url = base_url + "/llm/generate";

    json j_req = req;
    std::string body = j_req.dump();

    // Retry with linear backoff (500ms * attempt)
    core::HttpResponse resp;
    unsigned long attempts = 0;
    const unsigned long max_attempts = 2;

    while (attempts < max_attempts) {
        if (attempts > 0) {
            http->Delay(500 * attempts);
        }

        resp = http->Post(url, cached_headers_, body);

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
    if (j_resp.is_discarded()) {
        return nullptr;
    }

    auto chat_resp = std::make_shared<core::ChatResponse>();
    *chat_resp = j_resp.get<core::ChatResponse>();

    return chat_resp;
}

}  // namespace remote
}  // namespace provider
}  // namespace foresthub
