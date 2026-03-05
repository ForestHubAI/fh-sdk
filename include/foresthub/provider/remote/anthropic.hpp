#ifndef FORESTHUB_PROVIDER_REMOTE_ANTHROPIC_HPP
#define FORESTHUB_PROVIDER_REMOTE_ANTHROPIC_HPP

#include <memory>
#include <string>
#include <vector>

#include "foresthub/config/config.hpp"
#include "foresthub/core/http_client.hpp"
#include "foresthub/core/provider.hpp"

namespace foresthub {
namespace provider {
namespace remote {

/// Direct Anthropic Claude provider using the native Messages API (`POST /v1/messages`).
///
/// Supported options: temperature, max_tokens, top_p, top_k.
/// Anthropic requires max_tokens; defaults to 4096 when not explicitly set via `WithMaxTokens()`.
/// Unsupported options (seed, frequency_penalty, presence_penalty) are silently ignored.
///
/// Web search is NOT supported in this version. When a ChatRequest contains a WebSearch tool,
/// it is silently filtered out. Anthropic's server-side web search returns `server_tool_use`
/// and `web_search_tool_result` content blocks with `encrypted_content` tokens that must be
/// passed back verbatim in multi-turn conversations. The SDK's InputItems system does not
/// currently support opaque provider-specific content block passthrough, which would cause
/// HTTP 400 errors in multi-turn agent loops.
class AnthropicProvider : public core::Provider {
public:
    /// Construct an Anthropic provider from configuration.
    /// @param cfg Provider configuration (api_key, base_url, supported_models).
    /// @param http_client HTTP implementation for API calls.
    AnthropicProvider(const config::ProviderConfig& cfg, std::shared_ptr<core::HttpClient> http_client);

    /// Returns "anthropic" as the provider identifier.
    core::ProviderID ProviderId() const override;

    /// Check that the Anthropic API is reachable.
    /// @return Error message on failure, empty on success.
    std::string Health() const override;

    /// Send a chat request to the Anthropic Messages API.
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

#endif  // FORESTHUB_PROVIDER_REMOTE_ANTHROPIC_HPP
