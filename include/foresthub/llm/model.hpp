// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_LLM_MODEL_HPP
#define FORESTHUB_LLM_MODEL_HPP

/// @file
/// Model metadata and capability identifiers.

#include <string>
#include <vector>

#include "foresthub/util/optional.hpp"

namespace foresthub {
namespace llm {

/// Unique identifier for an LLM provider.
using ProviderID = std::string;

/// Unique identifier for an LLM model.
using ModelID = std::string;

/// Capability string describing what a model can do.
using ModelCapability = std::string;

/// Well-known model capability constants.
namespace capability {
static constexpr const char* kChat = "chat";                      ///< Basic conversational ability.
static constexpr const char* kFunctionCall = "function_call";     ///< Tool/function calling support.
static constexpr const char* kVision = "vision";                  ///< Image understanding capability.
static constexpr const char* kFineTuning = "fine_tuning";         ///< Model fine-tuning support.
static constexpr const char* kReasoning = "reasoning";            ///< Chain-of-thought reasoning.
static constexpr const char* kClassification = "classification";  ///< Text classification support.
static constexpr const char* kCode = "code";                      ///< Code generation and understanding.
}  // namespace capability

/// Metadata about an LLM model.
struct ModelInfo {
    ModelID id;                                 ///< Unique model identifier (e.g., "gpt-4o").
    ProviderID provider;                        ///< Provider that hosts this model.
    util::Optional<int> max_tokens;             ///< Maximum context length, if known.
    std::vector<ModelCapability> capabilities;  ///< Supported capabilities (chat, vision, etc.).
};

}  // namespace llm
}  // namespace foresthub

#endif  // FORESTHUB_LLM_MODEL_HPP
