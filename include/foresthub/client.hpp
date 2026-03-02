#ifndef FORESTHUB_CLIENT_HPP
#define FORESTHUB_CLIENT_HPP

#include <map>
#include <memory>
#include <string>

#include "foresthub/config/config.hpp"
#include "foresthub/core/http_client.hpp"
#include "foresthub/core/model.hpp"
#include "foresthub/core/provider.hpp"
#include "foresthub/core/types.hpp"

namespace foresthub {

/// Main entry point for interacting with multiple LLM providers.
///
/// Routes chat requests to the appropriate provider based on model ID.
class Client : public core::LLMClient {
public:
    /// Registered providers keyed by ProviderID.
    std::map<core::ProviderID, std::shared_ptr<core::Provider>> providers;

    /// Default constructor. Prefer Client::Create() for initialization.
    Client();

    /// Create and initialize a Client from configuration.
    /// @param cfg Client configuration with provider settings.
    /// @param http_client HTTP implementation to inject into providers.
    /// @return Configured client ready for use.
    static std::unique_ptr<Client> Create(const config::ClientConfig& cfg,
                                          const std::shared_ptr<core::HttpClient>& http_client = nullptr);

    /// Register a provider instance manually.
    /// @param provider Provider to add to the routing table; nullptr is silently ignored.
    void RegisterProvider(const std::shared_ptr<core::Provider>& provider);

    /// Check health of all registered providers.
    /// @return Error message on failure, empty on success.
    std::string Health() const;

    /// Check if any registered provider supports the given model.
    /// @param model Model identifier to check.
    /// @return True if at least one provider supports the model.
    bool SupportsModel(const core::ModelID& model) const;

    /// Send a chat request, auto-selecting the provider by model ID.
    /// @param req Chat request with model, input, and options.
    /// @return Chat response, or nullptr if no provider supports the model.
    std::shared_ptr<core::ChatResponse> Chat(const core::ChatRequest& req) override;

private:
    /// Find a provider that supports the requested model, or nullptr if none found.
    std::shared_ptr<core::Provider> InferProvider(const core::ModelID& model) const;
};

}  // namespace foresthub

#endif  // FORESTHUB_CLIENT_HPP
