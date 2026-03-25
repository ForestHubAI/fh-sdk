// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "foresthub/llm/options.hpp"

#include <gtest/gtest.h>

#include "foresthub/util/optional.hpp"

using namespace foresthub::llm;

// ==========================================
// 1. Tests for Default State
// ==========================================

TEST(OptionsTest, DefaultConstructorIsEmpty) {
    Options opts;

    EXPECT_FALSE(opts.max_tokens.HasValue());
    EXPECT_FALSE(opts.temperature.HasValue());
    EXPECT_FALSE(opts.top_k.HasValue());
    EXPECT_FALSE(opts.top_p.HasValue());
    EXPECT_FALSE(opts.frequency_penalty.HasValue());
    EXPECT_FALSE(opts.presence_penalty.HasValue());
    EXPECT_FALSE(opts.seed.HasValue());
}

// ==========================================
// 2. Tests for Builder Methods (Chaining)
// ==========================================

TEST(OptionsTest, BuilderMethodsChaining) {
    Options opts;

    opts.WithMaxTokens(1024).WithTemperature(0.7f).WithTopP(0.9f);

    ASSERT_TRUE(opts.max_tokens.HasValue());
    EXPECT_EQ(*opts.max_tokens, 1024);

    ASSERT_TRUE(opts.temperature.HasValue());
    EXPECT_FLOAT_EQ(*opts.temperature, 0.7f);

    ASSERT_TRUE(opts.top_p.HasValue());
    EXPECT_FLOAT_EQ(*opts.top_p, 0.9f);

    // Verify other fields remain empty
    EXPECT_FALSE(opts.top_k.HasValue());
    EXPECT_FALSE(opts.seed.HasValue());
}

// ==========================================
// 3. Tests for Individual Setters
// ==========================================

TEST(OptionsTest, IndividualSetters) {
    Options opts;

    opts.WithMaxTokens(500);
    ASSERT_TRUE(opts.max_tokens.HasValue());
    EXPECT_EQ(*opts.max_tokens, 500);

    opts.WithTemperature(0.5f);
    ASSERT_TRUE(opts.temperature.HasValue());
    EXPECT_FLOAT_EQ(*opts.temperature, 0.5f);

    opts.WithTopK(50);
    ASSERT_TRUE(opts.top_k.HasValue());
    EXPECT_EQ(*opts.top_k, 50);

    opts.WithTopP(0.85f);
    ASSERT_TRUE(opts.top_p.HasValue());
    EXPECT_FLOAT_EQ(*opts.top_p, 0.85f);

    opts.WithFrequencyPenalty(0.2f);
    ASSERT_TRUE(opts.frequency_penalty.HasValue());
    EXPECT_FLOAT_EQ(*opts.frequency_penalty, 0.2f);

    // Test with negative value (often valid in LLM APIs)
    opts.WithPresencePenalty(-0.5f);
    ASSERT_TRUE(opts.presence_penalty.HasValue());
    EXPECT_FLOAT_EQ(*opts.presence_penalty, -0.5f);

    opts.WithSeed(42);
    ASSERT_TRUE(opts.seed.HasValue());
    EXPECT_EQ(*opts.seed, 42);
}

// ==========================================
// 4. Tests for Modification Logic
// ==========================================

TEST(OptionsTest, OverwriteValues) {
    Options opts;

    opts.WithMaxTokens(100);
    EXPECT_EQ(*opts.max_tokens, 100);

    opts.WithMaxTokens(200);
    EXPECT_EQ(*opts.max_tokens, 200);
}

TEST(OptionsTest, DirectMemberAccess) {
    Options opts;

    opts.max_tokens = 123;
    ASSERT_TRUE(opts.max_tokens.HasValue());
    EXPECT_EQ(*opts.max_tokens, 123);

    opts.max_tokens = foresthub::Optional<int>{};
    EXPECT_FALSE(opts.max_tokens.HasValue());
}
