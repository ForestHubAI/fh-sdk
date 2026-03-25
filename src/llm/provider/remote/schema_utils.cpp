// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "schema_utils.hpp"

namespace foresthub {
namespace provider {
namespace remote {

json EnsureAllRequired(json schema) {
    if (!schema.is_object()) {
        return schema;
    }

    // Object type: collect all property keys into required.
    if (schema.value("type", "") == "object" && schema.contains("properties") && schema["properties"].is_object()) {
        json required = json::array();
        for (auto it = schema["properties"].begin(); it != schema["properties"].end(); ++it) {
            required.push_back(it.key());
            it.value() = EnsureAllRequired(it.value());
        }
        schema["required"] = std::move(required);
    }

    // Recurse into array items.
    if (schema.contains("items") && schema["items"].is_object()) {
        schema["items"] = EnsureAllRequired(schema["items"]);
    }

    return schema;
}

json SetNoAdditionalProperties(json schema) {
    if (!schema.is_object()) {
        return schema;
    }

    if (schema.value("type", "") == "object") {
        schema["additionalProperties"] = false;
    }

    if (schema.contains("properties") && schema["properties"].is_object()) {
        for (auto it = schema["properties"].begin(); it != schema["properties"].end(); ++it) {
            it.value() = SetNoAdditionalProperties(it.value());
        }
    }

    if (schema.contains("items") && schema["items"].is_object()) {
        schema["items"] = SetNoAdditionalProperties(schema["items"]);
    }

    return schema;
}

json StripAdditionalProperties(json schema) {
    if (!schema.is_object()) {
        return schema;
    }

    schema.erase("additionalProperties");

    if (schema.contains("properties") && schema["properties"].is_object()) {
        for (auto it = schema["properties"].begin(); it != schema["properties"].end(); ++it) {
            it.value() = StripAdditionalProperties(it.value());
        }
    }

    if (schema.contains("items") && schema["items"].is_object()) {
        schema["items"] = StripAdditionalProperties(schema["items"]);
    }

    return schema;
}

}  // namespace remote
}  // namespace provider
}  // namespace foresthub
