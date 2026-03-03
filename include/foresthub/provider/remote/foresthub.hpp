#ifndef FORESTHUB_PROVIDER_REMOTE_FOREST_HUB_HPP
#define FORESTHUB_PROVIDER_REMOTE_FOREST_HUB_HPP

#include <memory>
#include <string>
#include <vector>

#include "foresthub/config/config.hpp"
#include "foresthub/core/http_client.hpp"
#include "foresthub/core/provider.hpp"

namespace foresthub {
namespace provider {
namespace remote {

/// Provider implementation that communicates with the ForestHub backend via HTTP.
class ForestHubProvider : public foresthub::core::Provider {
public:
    /// Construct a ForestHub provider from configuration.
    /// @param cfg Provider configuration (api_key, base_url, supported_models).
    /// @param http_client HTTP implementation for API calls.
    ForestHubProvider(const config::ProviderConfig& cfg, std::shared_ptr<foresthub::core::HttpClient> http_client);

    /// Returns "forest-hub" as the provider identifier.
    core::ProviderID ProviderId() const override;

    /// Check that the remote provider is reachable and responding.
    /// @return Error message on failure, empty on success.
    std::string Health() const override;

    /// Send a chat completion request to the backend.
    /// @param req Chat request with model, input, and options.
    /// @return Chat response from the backend, or nullptr on error.
    std::shared_ptr<core::ChatResponse> Chat(const core::ChatRequest& req) override;

    /// Check if this provider lists the model as supported.
    /// @param model Model identifier to check.
    bool SupportsModel(const core::ModelID& model) const override;

private:
    std::shared_ptr<foresthub::core::HttpClient> http_;
    std::string api_key_;
    std::string base_url_;
    std::vector<std::string> supported_models_;
    /// Pre-built HTTP headers (Content-Type, Authorization) set once in constructor.
    core::HttpClient::Headers cached_headers_;
};

}  // namespace remote
}  // namespace provider
}  // namespace foresthub

#endif  // FORESTHUB_PROVIDER_REMOTE_FOREST_HUB_HPP
