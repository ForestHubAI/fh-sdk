// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "foresthub/util/schema.hpp"

#include <gtest/gtest.h>

namespace foresthub {
namespace util {
namespace {

TEST(NormalizeSchema, MinimalFormat) {
    json input = {{"city", {{"type", "string"}}}, {"units", {{"type", "string"}}}};
    json result = NormalizeSchema(input);

    EXPECT_EQ(result.value("type", ""), "object");
    ASSERT_TRUE(result.contains("properties"));
    EXPECT_TRUE(result["properties"].contains("city"));
    EXPECT_TRUE(result["properties"].contains("units"));
}

TEST(NormalizeSchema, FullFormat) {
    json input = {{"type", "object"}, {"properties", {{"city", {{"type", "string"}}}}}};
    json result = NormalizeSchema(input);

    EXPECT_EQ(result, input);
}

TEST(NormalizeSchema, Empty) {
    json result = NormalizeSchema(json::object());

    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result.empty());
}

TEST(NormalizeSchema, Null) {
    json result = NormalizeSchema(json());

    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result.empty());
}

TEST(NormalizeSchema, NonObjectType) {
    json input = {{"type", "string"}};
    json result = NormalizeSchema(input);

    EXPECT_EQ(result, input);
}

TEST(NormalizeSchema, NestedPreserved) {
    json input = {{"address", {{"type", "object"}, {"properties", {{"street", {{"type", "string"}}}}}}}};
    json result = NormalizeSchema(input);

    EXPECT_EQ(result["properties"]["address"]["type"], "object");
    EXPECT_TRUE(result["properties"]["address"]["properties"].contains("street"));
}

TEST(NormalizeSchema, PropertiesWithoutType) {
    json input = {{"properties", {{"city", {{"type", "string"}}}}}, {"required", {"city"}}};
    json result = NormalizeSchema(input);

    EXPECT_EQ(result.value("type", ""), "object");
    ASSERT_TRUE(result.contains("properties"));
    EXPECT_TRUE(result["properties"].contains("city"));
    EXPECT_TRUE(result.contains("required"));
}

}  // namespace
}  // namespace util
}  // namespace foresthub
