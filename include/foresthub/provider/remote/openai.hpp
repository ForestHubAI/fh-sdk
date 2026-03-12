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

#include "foresthub/config/config.hpp"
#include "foresthub/core/http_client.hpp"
#include "foresthub/core/provider.hpp"

namespace foresthub {
namespace provider {
namespace remote {

/// Direct OpenAI provider using the Responses API (`POST /v1/responses`).
class OpenAIProvider : public core::Provider {
public:
    /// Construct an OpenAI provider from configuration.
    /// @param cfg Provider configuration (api_key, base_url, supported_models).
    /// @param http_client HTTP implementation for API calls.
    OpenAIProvider(const config::ProviderConfig& cfg, std::shared_ptr<core::HttpClient> http_client);

    /// Returns "openai" as the provider identifier.
    core::ProviderID ProviderId() const override;

    /// Check that the OpenAI API is reachable.
    /// @return Error message on failure, empty on success.
    std::string Health() const override;

    /// Send a chat request to the OpenAI Responses API.
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

#endif  // FORESTHUB_PROVIDER_REMOTE_OPENAI_HPP
