#ifndef FORESTHUB_PROVIDER_REMOTE_ANTHROPIC_MAPPING_HPP
#define FORESTHUB_PROVIDER_REMOTE_ANTHROPIC_MAPPING_HPP

#include <memory>

#include "foresthub/core/types.hpp"
#include "foresthub/util/json.hpp"

namespace foresthub {
namespace provider {
namespace remote {

using json = nlohmann::json;

/// Convert a ChatRequest to Anthropic Messages API JSON format.
json ToAnthropicRequest(const core::ChatRequest& req, int default_max_tokens);

/// Parse an Anthropic Messages API JSON response into a ChatResponse.
std::shared_ptr<core::ChatResponse> FromAnthropicResponse(const json& j);

}  // namespace remote
}  // namespace provider
}  // namespace foresthub

#endif  // FORESTHUB_PROVIDER_REMOTE_ANTHROPIC_MAPPING_HPP
