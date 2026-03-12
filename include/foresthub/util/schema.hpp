#ifndef FORESTHUB_UTIL_SCHEMA_HPP
#define FORESTHUB_UTIL_SCHEMA_HPP

/// @file
/// JSON Schema normalization utilities.

#include "foresthub/util/json.hpp"

namespace foresthub {
namespace util {

using json = nlohmann::json;

/// Wraps a minimal properties-only JSON object into a full JSON Schema.
///
/// Handles four input formats:
/// - Empty/null → returns empty object.
/// - Already has `"type"` → returned unchanged.
/// - Has `"properties"` but no `"type"` → adds `"type": "object"`.
/// - Otherwise → treated as a bare property map, wrapped into `{"type":"object","properties":...}`.
/// @param schema Raw JSON schema in any of the supported formats.
/// @return Normalized JSON Schema object.
json NormalizeSchema(json schema);

}  // namespace util
}  // namespace foresthub

#endif  // FORESTHUB_UTIL_SCHEMA_HPP
