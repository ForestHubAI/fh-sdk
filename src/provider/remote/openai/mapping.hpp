#ifndef FORESTHUB_PROVIDER_REMOTE_OPENAI_MAPPING_HPP
#define FORESTHUB_PROVIDER_REMOTE_OPENAI_MAPPING_HPP

#include <memory>

#include "foresthub/core/types.hpp"
#include "foresthub/util/json.hpp"

namespace foresthub {
namespace provider {
namespace remote {

/// Convert a ChatRequest to OpenAI Responses API JSON format.
nlohmann::json ToOpenAIRequest(const core::ChatRequest& req);

/// Parse an OpenAI Responses API JSON response into a ChatResponse.
std::shared_ptr<core::ChatResponse> FromOpenAIResponse(const nlohmann::json& j);

}  // namespace remote
}  // namespace provider
}  // namespace foresthub

#endif  // FORESTHUB_PROVIDER_REMOTE_OPENAI_MAPPING_HPP
