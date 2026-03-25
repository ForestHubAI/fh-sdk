// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PROVIDER_REMOTE_GEMINI_HPP
#define FORESTHUB_PROVIDER_REMOTE_GEMINI_HPP

/// @file
/// Google Gemini generateContent API provider implementation.

#include <memory>
#include <string>
#include <vector>

#include "foresthub/hal/http_client.hpp"
#include "foresthub/llm/config.hpp"
#include "foresthub/llm/provider.hpp"

namespace foresthub {
namespace provider {
namespace remote {

/// Direct Google Gemini provider using the native generateContent API (v1beta).
///
/// Google Search Grounding is supported via the `WebSearch` tool type, but it
/// cannot be combined with function calling in the same request. When both are
/// present, function calling takes priority and Google Search is silently dropped.
class GeminiProvider : public llm::Provider {
public:
    /// Construct a Gemini provider from configuration.
    /// @param cfg Provider configuration (api_key, base_url, supported_models).
    /// @param http_client HTTP implementation for API calls.
    GeminiProvider(const llm::ProviderConfig& cfg, std::shared_ptr<hal::HttpClient> http_client);

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

}  // namespace remote
}  // namespace provider
}  // namespace foresthub

#endif  // FORESTHUB_PROVIDER_REMOTE_GEMINI_HPP
