#ifndef FORESTHUB_PROVIDER_REMOTE_GEMINI_MAPPING_HPP
#define FORESTHUB_PROVIDER_REMOTE_GEMINI_MAPPING_HPP

#include <memory>

#include "foresthub/core/types.hpp"
#include "foresthub/util/json.hpp"

namespace foresthub {
namespace provider {
namespace remote {

/// Convert a ChatRequest to Gemini generateContent API JSON format.
nlohmann::json ToGeminiRequest(const core::ChatRequest& req);

/// Parse a Gemini generateContent API JSON response into a ChatResponse.
std::shared_ptr<core::ChatResponse> FromGeminiResponse(const nlohmann::json& j);

}  // namespace remote
}  // namespace provider
}  // namespace foresthub

#endif  // FORESTHUB_PROVIDER_REMOTE_GEMINI_MAPPING_HPP
