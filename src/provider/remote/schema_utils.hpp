#ifndef FORESTHUB_PROVIDER_REMOTE_SCHEMA_UTILS_HPP
#define FORESTHUB_PROVIDER_REMOTE_SCHEMA_UTILS_HPP

#include "foresthub/util/json.hpp"

namespace foresthub {
namespace provider {
namespace remote {

using json = nlohmann::json;

/// Overwrites the "required" array with ALL property keys, recursively.
/// Non-object schemas pass through unchanged.
json EnsureAllRequired(json schema);

/// Sets "additionalProperties": false on all object types, recursively.
/// Existing values are silently overwritten.
json SetNoAdditionalProperties(json schema);

/// Removes the "additionalProperties" key from all objects, recursively.
json StripAdditionalProperties(json schema);

}  // namespace remote
}  // namespace provider
}  // namespace foresthub

#endif  // FORESTHUB_PROVIDER_REMOTE_SCHEMA_UTILS_HPP
