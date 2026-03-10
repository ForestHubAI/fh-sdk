#include "provider/remote/schema_utils.hpp"

#include <gtest/gtest.h>

namespace foresthub {
namespace provider {
namespace remote {
namespace {

// --- EnsureAllRequired ---

TEST(SchemaUtils, EnsureAllRequiredFlat) {
    json input = {{"type", "object"},
                  {"properties", {{"city", {{"type", "string"}}}, {"units", {{"type", "string"}}}}}};
    json result = EnsureAllRequired(input);

    ASSERT_TRUE(result.contains("required"));
    ASSERT_EQ(result["required"].size(), 2u);
    // All property keys must be present in required.
    bool has_city = false;
    bool has_units = false;
    for (const auto& key : result["required"]) {
        if (key == "city") has_city = true;
        if (key == "units") has_units = true;
    }
    EXPECT_TRUE(has_city);
    EXPECT_TRUE(has_units);
}

TEST(SchemaUtils, EnsureAllRequiredOverwritesPartial) {
    json input = {{"type", "object"},
                  {"properties", {{"city", {{"type", "string"}}}, {"units", {{"type", "string"}}}}},
                  {"required", {"city"}}};
    json result = EnsureAllRequired(input);

    ASSERT_EQ(result["required"].size(), 2u);
}

TEST(SchemaUtils, EnsureAllRequiredNested) {
    json input = {
        {"type", "object"},
        {"properties",
         {{"address",
           {{"type", "object"}, {"properties", {{"street", {{"type", "string"}}}, {"zip", {{"type", "string"}}}}}}}}}};
    json result = EnsureAllRequired(input);

    ASSERT_TRUE(result.contains("required"));
    ASSERT_TRUE(result["properties"]["address"].contains("required"));
    EXPECT_EQ(result["properties"]["address"]["required"].size(), 2u);
}

TEST(SchemaUtils, EnsureAllRequiredNonObject) {
    json input = {{"type", "string"}};
    json result = EnsureAllRequired(input);

    EXPECT_EQ(result, input);
    EXPECT_FALSE(result.contains("required"));
}

TEST(SchemaUtils, EnsureAllRequiredArrayItems) {
    json input = {{"type", "array"}, {"items", {{"type", "object"}, {"properties", {{"name", {{"type", "string"}}}}}}}};
    json result = EnsureAllRequired(input);

    ASSERT_TRUE(result["items"].contains("required"));
    EXPECT_EQ(result["items"]["required"].size(), 1u);
}

TEST(SchemaUtils, EnsureAllRequiredEmptyProperties) {
    json input = {{"type", "object"}, {"properties", json::object()}};
    json result = EnsureAllRequired(input);

    ASSERT_TRUE(result.contains("required"));
    EXPECT_TRUE(result["required"].empty());
}

// --- SetNoAdditionalProperties ---

TEST(SchemaUtils, SetNoAdditionalPropertiesFlat) {
    json input = {{"type", "object"}, {"properties", {{"city", {{"type", "string"}}}}}};
    json result = SetNoAdditionalProperties(input);

    EXPECT_EQ(result["additionalProperties"], false);
}

TEST(SchemaUtils, SetNoAdditionalPropertiesNested) {
    json input = {
        {"type", "object"},
        {"properties", {{"address", {{"type", "object"}, {"properties", {{"street", {{"type", "string"}}}}}}}}}};
    json result = SetNoAdditionalProperties(input);

    EXPECT_EQ(result["additionalProperties"], false);
    EXPECT_EQ(result["properties"]["address"]["additionalProperties"], false);
}

TEST(SchemaUtils, SetNoAdditionalPropertiesOverwritesTrue) {
    json input = {{"type", "object"}, {"properties", json::object()}, {"additionalProperties", true}};
    json result = SetNoAdditionalProperties(input);

    EXPECT_EQ(result["additionalProperties"], false);
}

TEST(SchemaUtils, SetNoAdditionalPropertiesNonObject) {
    json input = {{"type", "string"}};
    json result = SetNoAdditionalProperties(input);

    EXPECT_FALSE(result.contains("additionalProperties"));
}

TEST(SchemaUtils, SetNoAdditionalPropertiesArrayItems) {
    json input = {{"type", "array"}, {"items", {{"type", "object"}, {"properties", {{"name", {{"type", "string"}}}}}}}};
    json result = SetNoAdditionalProperties(input);

    EXPECT_FALSE(result.contains("additionalProperties"));
    EXPECT_EQ(result["items"]["additionalProperties"], false);
}

// --- StripAdditionalProperties ---

TEST(SchemaUtils, StripAdditionalPropertiesPresent) {
    json input = {{"type", "object"}, {"properties", json::object()}, {"additionalProperties", false}};
    json result = StripAdditionalProperties(input);

    EXPECT_FALSE(result.contains("additionalProperties"));
}

TEST(SchemaUtils, StripAdditionalPropertiesAbsent) {
    json input = {{"type", "object"}, {"properties", json::object()}};
    json result = StripAdditionalProperties(input);

    EXPECT_FALSE(result.contains("additionalProperties"));
    EXPECT_EQ(result.value("type", ""), "object");
}

TEST(SchemaUtils, StripAdditionalPropertiesNested) {
    json input = {
        {"type", "object"},
        {"properties",
         {{"address", {{"type", "object"}, {"properties", json::object()}, {"additionalProperties", false}}}}},
        {"additionalProperties", false}};
    json result = StripAdditionalProperties(input);

    EXPECT_FALSE(result.contains("additionalProperties"));
    EXPECT_FALSE(result["properties"]["address"].contains("additionalProperties"));
}

TEST(SchemaUtils, StripAdditionalPropertiesNonObject) {
    json input = {{"type", "string"}};
    json result = StripAdditionalProperties(input);

    EXPECT_EQ(result, input);
}

TEST(SchemaUtils, StripAdditionalPropertiesArrayItems) {
    json input = {{"type", "array"},
                  {"items", {{"type", "object"}, {"properties", json::object()}, {"additionalProperties", false}}}};
    json result = StripAdditionalProperties(input);

    EXPECT_FALSE(result["items"].contains("additionalProperties"));
}

// --- Non-JSON-object inputs (literal string/number/array) ---

TEST(SchemaUtils, EnsureAllRequiredLiteralNonObject) {
    json str = json("just a string");
    EXPECT_EQ(EnsureAllRequired(str), str);

    json num = json(42);
    EXPECT_EQ(EnsureAllRequired(num), num);
}

TEST(SchemaUtils, SetNoAdditionalPropertiesLiteralNonObject) {
    json str = json("just a string");
    EXPECT_EQ(SetNoAdditionalProperties(str), str);

    json arr = json::array({1, 2, 3});
    EXPECT_EQ(SetNoAdditionalProperties(arr), arr);
}

TEST(SchemaUtils, StripAdditionalPropertiesLiteralNonObject) {
    json str = json("just a string");
    EXPECT_EQ(StripAdditionalProperties(str), str);

    json null_val = json(nullptr);
    EXPECT_EQ(StripAdditionalProperties(null_val), null_val);
}

}  // namespace
}  // namespace remote
}  // namespace provider
}  // namespace foresthub
