// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PROVIDER_REMOTE_ANTHROPIC_HPP
#define FORESTHUB_PROVIDER_REMOTE_ANTHROPIC_HPP

/// @file
/// Anthropic Messages API provider implementation.

#include <memory>
#include <string>
#include <vector>

#include "foresthub/hal/http_client.hpp"
#include "foresthub/llm/config.hpp"
#include "foresthub/llm/provider.hpp"

namespace foresthub {
namespace llm {
namespace anthropic {

/// Direct Anthropic Claude provider using the native Messages API (`POST /v1/messages`).
///
/// Supported options: temperature, max_tokens, top_p, top_k.
/// Anthropic requires max_tokens; defaults to 4096 when not explicitly set via `WithMaxTokens()`.
/// Unsupported options (seed, frequency_penalty, presence_penalty) are silently ignored.
///
/// Web search is NOT supported in this version. When a ChatRequest contains a WebSearch tool,
/// it is silently filtered out.
class AnthropicProvider : public llm::Provider {
public:
    /// Construct an Anthropic provider from configuration.
    /// @param cfg Provider configuration (api_key, base_url, supported_models).
    /// @param http_client HTTP implementation for API calls.
    AnthropicProvider(const llm::ProviderConfig& cfg, std::shared_ptr<hal::HttpClient> http_client);

    llm::ProviderID ProviderId() const override;
    std::string Health() const override;
    std::shared_ptr<llm::ChatResponse> Chat(const llm::ChatRequest& req) override;
    bool SupportsModel(const llm::ModelID& model) const override;

private:
    std::shared_ptr<hal::HttpClient> http_;
    std::string api_key_;
    std::string base_url_;
    std::vector<llm::ModelID> supported_models_;
    hal::HttpClient::Headers cached_headers_;
};

}  // namespace anthropic
}  // namespace llm
}  // namespace foresthub

#endif  // FORESTHUB_PROVIDER_REMOTE_ANTHROPIC_HPP
