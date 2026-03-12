// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PROVIDER_REMOTE_OPENAI_MAPPING_HPP
#define FORESTHUB_PROVIDER_REMOTE_OPENAI_MAPPING_HPP

#include <memory>

#include "foresthub/core/types.hpp"
#include "foresthub/util/json.hpp"

namespace foresthub {
namespace provider {
namespace remote {

using json = nlohmann::json;

/// Convert a ChatRequest to OpenAI Responses API JSON format.
json ToOpenAIRequest(const core::ChatRequest& req);

/// Parse an OpenAI Responses API JSON response into a ChatResponse.
std::shared_ptr<core::ChatResponse> FromOpenAIResponse(const json& j);

}  // namespace remote
}  // namespace provider
}  // namespace foresthub

#endif  // FORESTHUB_PROVIDER_REMOTE_OPENAI_MAPPING_HPP
