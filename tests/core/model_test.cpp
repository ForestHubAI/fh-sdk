#include "foresthub/core/model.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "foresthub/util/optional.hpp"

using namespace foresthub::core;

// ==========================================
// 1. Tests for Capability Constants
// ==========================================

TEST(ModelCapabilityTest, ConstantsMatchExpectedValues) {
    EXPECT_EQ(capability::kChat, "chat");
    EXPECT_EQ(capability::kFunctionCall, "function_call");
    EXPECT_EQ(capability::kVision, "vision");
    EXPECT_EQ(capability::kFineTuning, "fine_tuning");
    EXPECT_EQ(capability::kReasoning, "reasoning");
    EXPECT_EQ(capability::kClassification, "classification");
    EXPECT_EQ(capability::kCode, "code");
}

// ==========================================
// 2. Tests for ModelInfo Struct
// ==========================================

TEST(ModelInfoTest, DefaultConstructor) {
    ModelInfo info;

    EXPECT_TRUE(info.id.empty());
    EXPECT_TRUE(info.provider.empty());
    EXPECT_FALSE(info.max_tokens.HasValue());
    EXPECT_TRUE(info.capabilities.empty());
}

TEST(ModelInfoTest, MemberAccessAndModification) {
    ModelInfo info;

    std::string expected_id = "gpt-4-turbo";
    std::string expected_provider = "OpenAI";

    info.id = expected_id;
    info.provider = expected_provider;

    EXPECT_EQ(info.id, expected_id);
    EXPECT_EQ(info.provider, expected_provider);

    EXPECT_FALSE(info.max_tokens.HasValue());

    int token_limit = 128000;
    info.max_tokens = token_limit;

    ASSERT_TRUE(info.max_tokens.HasValue());
    EXPECT_EQ(*info.max_tokens, token_limit);
}

TEST(ModelInfoTest, CapabilityVectorManipulation) {
    ModelInfo info;

    // Explicitly construct std::string from const char* because the vector holds std::string
    info.capabilities.push_back(std::string(capability::kChat));
    info.capabilities.push_back(std::string(capability::kVision));

    ASSERT_EQ(info.capabilities.size(), 2);
    EXPECT_EQ(info.capabilities[0], "chat");
    EXPECT_EQ(info.capabilities[1], "vision");

    info.capabilities.clear();
    EXPECT_EQ(info.capabilities.size(), 0);
}

TEST(ModelInfoTest, AggregateInitialization) {
    // Syntax: { id, provider, max_tokens, capabilities }
    ModelInfo info = {"mistral-large",  // id
                      "Mistral",        // provider
                      32000,            // max_tokens (implicit conversion to optional)
                      {                 // capabilities (vector initializer list)
                       std::string(capability::kChat), std::string(capability::kFunctionCall)}};

    EXPECT_EQ(info.id, "mistral-large");
    EXPECT_EQ(info.provider, "Mistral");

    ASSERT_TRUE(info.max_tokens.HasValue());
    EXPECT_EQ(*info.max_tokens, 32000);

    ASSERT_EQ(info.capabilities.size(), 2);
    EXPECT_EQ(info.capabilities[0], "chat");
    EXPECT_EQ(info.capabilities[1], "function_call");
}

TEST(ModelInfoTest, AggregateInitializationNullTokens) {
    ModelInfo info = {
        "text-embedding-3",
        "OpenAI",
        foresthub::Optional<int>{},  // No max tokens info
        {}                           // Empty capabilities
    };

    EXPECT_EQ(info.id, "text-embedding-3");
    EXPECT_FALSE(info.max_tokens.HasValue());
    EXPECT_TRUE(info.capabilities.empty());
}
