// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_CORE_SERIALIZATION_HPP
#define FORESTHUB_CORE_SERIALIZATION_HPP

/// @file
/// JSON serialization and deserialization for core types.

#include <memory>

#include "foresthub/llm/input.hpp"
#include "foresthub/llm/options.hpp"
#include "foresthub/llm/tools.hpp"
#include "foresthub/llm/types.hpp"
#include "foresthub/util/json.hpp"

namespace foresthub {
namespace llm {

using json = nlohmann::json;

// -- Core structure serialization --

/// Serialize Options to JSON.
void to_json(json& j, const Options& opts);
/// Deserialize Options from JSON.
void from_json(const json& j, Options& opts);
/// Serialize ResponseFormat to JSON.
void to_json(json& j, const ResponseFormat& format);
/// Deserialize ResponseFormat from JSON.
void from_json(const json& j, ResponseFormat& format);

// -- Polymorphic serialization helpers --

/// Serialize an InputItem (dispatches by subtype; nullptr produces JSON null).
/// @param item InputItem to serialize, or nullptr for null.
void to_json(json& j, const std::shared_ptr<InputItem>& item);
/// Serialize a Tool (dispatches by subtype).
void to_json(json& j, const std::shared_ptr<Tool>& tool);

// -- Request/Response serialization --

/// Serialize ChatRequest to JSON.
void to_json(json& j, const ChatRequest& req);
/// Deserialize ChatRequest from JSON.
void from_json(const json& j, ChatRequest& req);
/// Deserialize ChatResponse from JSON.
void from_json(const json& j, ChatResponse& resp);

}  // namespace llm
}  // namespace foresthub

#endif  // FORESTHUB_CORE_SERIALIZATION_HPP
