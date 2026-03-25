// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_LLM_PROVIDER_HPP
#define FORESTHUB_LLM_PROVIDER_HPP

/// @file
/// ChatClient and Provider interfaces for chat operations.

#include <memory>
#include <string>

#include "foresthub/llm/model.hpp"
#include "foresthub/llm/types.hpp"

namespace foresthub {
namespace llm {

/// Abstract interface for LLM chat operations.
class ChatClient {
public:
    virtual ~ChatClient() = default;

    /// Perform a chat completion request.
    /// @param req Chat request containing model, input, and generation options.
    /// @return Chat response, or nullptr on network/provider error.
    virtual std::shared_ptr<ChatResponse> Chat(const ChatRequest& req) = 0;
};

/// Extended LLM interface with health checks and model discovery.
class Provider : public ChatClient {
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

}  // namespace llm
}  // namespace foresthub

#endif  // FORESTHUB_LLM_PROVIDER_HPP
