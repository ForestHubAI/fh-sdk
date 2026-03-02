#ifndef FORESTHUB_CORE_PROVIDER_HPP
#define FORESTHUB_CORE_PROVIDER_HPP

#include <memory>
#include <string>

#include "foresthub/core/model.hpp"
#include "foresthub/core/types.hpp"

namespace foresthub {
namespace core {

/// Abstract interface for LLM chat operations.
class LLMClient {
public:
    virtual ~LLMClient() = default;

    /// Perform a chat completion request.
    /// @param req Chat request containing model, input, and generation options.
    /// @return Chat response, or nullptr on network/provider error.
    virtual std::shared_ptr<ChatResponse> Chat(const ChatRequest& req) = 0;
};

/// Extended LLM interface with health checks and model discovery.
class Provider : public LLMClient {
public:
    virtual ~Provider() = default;

    /// Returns the unique identifier of this provider.
    virtual ProviderID ProviderId() const = 0;

    /// Verify the LLM service is available.
    /// @return Error message on failure, empty on success.
    virtual std::string Health() const = 0;

    /// Check if this provider supports the given model.
    /// @param model Model identifier to check.
    /// @return True if this provider can handle requests for the given model.
    virtual bool SupportsModel(const ModelID& model) const = 0;
};

}  // namespace core
}  // namespace foresthub

#endif  // FORESTHUB_CORE_PROVIDER_HPP
