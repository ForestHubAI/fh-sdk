// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PROVIDER_REMOTE_FORESTHUB_HPP
#define FORESTHUB_PROVIDER_REMOTE_FORESTHUB_HPP

/// @file
/// ForestHub backend provider implementation.

#include <memory>
#include <string>
#include <vector>

#include "foresthub/hal/http_client.hpp"
#include "foresthub/llm/config.hpp"
#include "foresthub/llm/provider.hpp"

namespace foresthub {
namespace llm {
namespace fh {

/// Provider implementation that communicates with the ForestHub backend via HTTP.
class ForestHubProvider : public llm::Provider {
public:
    /// Construct a ForestHub provider from configuration.
    /// @param cfg Provider configuration (api_key, base_url, supported_models).
    /// @param http_client HTTP implementation for API calls.
    ForestHubProvider(const llm::ProviderConfig& cfg, std::shared_ptr<hal::HttpClient> http_client);

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

}  // namespace fh
}  // namespace llm
}  // namespace foresthub

#endif  // FORESTHUB_PROVIDER_REMOTE_FORESTHUB_HPP
