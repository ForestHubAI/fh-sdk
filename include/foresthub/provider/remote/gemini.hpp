#ifndef FORESTHUB_PROVIDER_REMOTE_GEMINI_HPP
#define FORESTHUB_PROVIDER_REMOTE_GEMINI_HPP

#include <memory>
#include <string>
#include <vector>

#include "foresthub/config/config.hpp"
#include "foresthub/core/http_client.hpp"
#include "foresthub/core/provider.hpp"

namespace foresthub {
namespace provider {
namespace remote {

/// Direct Google Gemini provider using the native generateContent API (v1beta).
///
/// Google Search Grounding is supported via the `WebSearch` tool type, but it
/// cannot be combined with function calling in the same request. When both are
/// present, function calling takes priority and Google Search is silently dropped.
class GeminiProvider : public core::Provider {
public:
    /// Construct a Gemini provider from configuration.
    /// @param cfg Provider configuration (api_key, base_url, supported_models).
    /// @param http_client HTTP implementation for API calls.
    GeminiProvider(const config::ProviderConfig& cfg, std::shared_ptr<core::HttpClient> http_client);

    /// Returns "gemini" as the provider identifier.
    core::ProviderID ProviderId() const override;

    /// Check that the Gemini API is reachable.
    /// @return Error message on failure, empty on success.
    std::string Health() const override;

    /// Send a chat request to the Gemini generateContent API.
    /// @param req Chat request with model, input, and options.
    /// @return Chat response, or nullptr on error.
    std::shared_ptr<core::ChatResponse> Chat(const core::ChatRequest& req) override;

    /// Check if this provider handles the given model.
    /// @param model Model identifier to check.
    /// @return True if supported_models is empty (accept all) or model is in the list.
    bool SupportsModel(const core::ModelID& model) const override;

private:
    std::shared_ptr<core::HttpClient> http_;
    std::string api_key_;
    std::string base_url_;
    std::vector<core::ModelID> supported_models_;
    core::HttpClient::Headers cached_headers_;
};

}  // namespace remote
}  // namespace provider
}  // namespace foresthub

#endif  // FORESTHUB_PROVIDER_REMOTE_GEMINI_HPP
