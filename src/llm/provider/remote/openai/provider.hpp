// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PROVIDER_REMOTE_OPENAI_HPP
#define FORESTHUB_PROVIDER_REMOTE_OPENAI_HPP

/// @file
/// OpenAI Responses API provider implementation.

#include <memory>
#include <string>
#include <vector>

#include "foresthub/hal/http_client.hpp"
#include "foresthub/llm/config.hpp"
#include "foresthub/llm/provider.hpp"

namespace foresthub {
namespace provider {
namespace remote {

/// Direct OpenAI provider using the Responses API (`POST /v1/responses`).
class OpenAIProvider : public llm::Provider {
public:
    /// Construct an OpenAI provider from configuration.
    /// @param cfg Provider configuration (api_key, base_url, supported_models).
    /// @param http_client HTTP implementation for API calls.
    OpenAIProvider(const llm::ProviderConfig& cfg, std::shared_ptr<hal::HttpClient> http_client);

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

#endif  // FORESTHUB_PROVIDER_REMOTE_OPENAI_HPP
