#include "foresthub/rag/remote/retriever.hpp"

#include <utility>

#include "foresthub/util/json.hpp"
#include "rag/serialization.hpp"

namespace foresthub {
namespace rag {
namespace remote {

using json = nlohmann::json;

RemoteRetriever::RemoteRetriever(const config::ProviderConfig& cfg, std::shared_ptr<core::HttpClient> http_client)
    : http_(std::move(http_client)), api_key_(cfg.api_key), base_url_(cfg.base_url) {
    // Strip trailing slash to avoid double slashes when appending paths.
    if (!base_url_.empty() && base_url_.back() == '/') {
        base_url_.pop_back();
    }
    cached_headers_ = {{"Content-Type", "application/json"}, {"Device-Key", api_key_}, {"Accept", "application/json"}};
}

std::shared_ptr<QueryResponse> RemoteRetriever::Query(const QueryRequest& req) {
    std::string url = base_url_ + "/rag/query";

    json j_req = req;
    std::string body = j_req.dump();

    // Retry with linear backoff (500ms * attempt).
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

        // 4xx: don't retry except for rate limits (429) and timeouts (408).
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
    if (j_resp.is_discarded() || !j_resp.is_array()) {
        return nullptr;
    }

    auto response = std::make_shared<QueryResponse>();
    for (const auto& item : j_resp) {
        if (!item.is_object()) {
            return nullptr;
        }
        response->results.push_back(item.get<QueryResult>());
    }

    return response;
}

}  // namespace remote
}  // namespace rag
}  // namespace foresthub
