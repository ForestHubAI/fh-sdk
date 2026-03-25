// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_LLM_CLIENT_HPP
#define FORESTHUB_LLM_CLIENT_HPP

/// @file
/// Multi-provider Client that routes chat requests by model ID.

#include <map>
#include <memory>
#include <string>

#include "foresthub/hal/http_client.hpp"
#include "foresthub/llm/config.hpp"
#include "foresthub/llm/model.hpp"
#include "foresthub/llm/provider.hpp"
#include "foresthub/llm/types.hpp"

namespace foresthub {
namespace llm {

/// Main entry point for interacting with multiple LLM providers.
///
/// Routes chat requests to the appropriate provider based on model ID.
class Client : public llm::ChatClient {
public:
    /// Default constructor. Prefer Client::Create() for initialization.
    Client();

    /// Create and initialize a Client from configuration.
    /// @param cfg Client configuration with provider settings.
    /// @param http_client HTTP implementation to inject into providers.
    /// @return Configured client ready for use.
    static std::unique_ptr<Client> Create(const llm::ClientConfig& cfg,
                                          const std::shared_ptr<hal::HttpClient>& http_client = nullptr);

    /// Register a provider instance manually.
    /// @param provider Provider to add to the routing table; nullptr is silently ignored.
    void RegisterProvider(const std::shared_ptr<llm::Provider>& provider);

    /// Check health of all registered providers.
    /// @return Error message on failure, empty on success.
    std::string Health() const;

    /// Check if any registered provider supports the given model.
    /// @param model Model identifier to check.
    /// @return True if at least one provider supports the model.
    bool SupportsModel(const llm::ModelID& model) const;

    /// Send a chat request, auto-selecting the provider by model ID.
    /// @param req Chat request with model, input, and options.
    /// @return Chat response, or nullptr if no provider supports the model.
    std::shared_ptr<llm::ChatResponse> Chat(const llm::ChatRequest& req) override;

    /// Read-only access to the registered providers.
    const std::map<llm::ProviderID, std::shared_ptr<llm::Provider>>& providers() const { return providers_; }

private:
    std::map<llm::ProviderID, std::shared_ptr<llm::Provider>> providers_;

    /// Find a provider that supports the requested model, or nullptr if none found.
    std::shared_ptr<llm::Provider> InferProvider(const llm::ModelID& model) const;
};

}  // namespace llm
}  // namespace foresthub

#endif  // FORESTHUB_LLM_CLIENT_HPP
