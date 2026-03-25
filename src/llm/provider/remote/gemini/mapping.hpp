// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PROVIDER_REMOTE_GEMINI_MAPPING_HPP
#define FORESTHUB_PROVIDER_REMOTE_GEMINI_MAPPING_HPP

#include <memory>

#include "foresthub/llm/types.hpp"
#include "foresthub/util/json.hpp"

namespace foresthub {
namespace llm {
namespace gemini {

using json = nlohmann::json;

/// Convert a ChatRequest to Gemini generateContent API JSON format.
json ToGeminiRequest(const llm::ChatRequest& req);

/// Parse a Gemini generateContent API JSON response into a ChatResponse.
std::shared_ptr<llm::ChatResponse> FromGeminiResponse(const json& j);

}  // namespace gemini
}  // namespace llm
}  // namespace foresthub

#endif  // FORESTHUB_PROVIDER_REMOTE_GEMINI_MAPPING_HPP
