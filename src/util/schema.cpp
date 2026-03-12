// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "foresthub/util/schema.hpp"

#include <utility>

namespace foresthub {
namespace util {

json NormalizeSchema(json schema) {
    if (schema.empty()) {
        return json::object();
    }
    if (schema.contains("type")) {
        return schema;
    }
    // A schema with "properties" is already full format, just missing "type".
    if (schema.contains("properties")) {
        schema["type"] = "object";
        return schema;
    }
    // Treat as minimal format: just a properties map.
    return json{{"type", "object"}, {"properties", std::move(schema)}};
}

}  // namespace util
}  // namespace foresthub
