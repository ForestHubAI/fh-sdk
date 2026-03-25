// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "foresthub/util/optional.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "foresthub/llm/types.hpp"

using namespace foresthub;
using util::Optional;

// ==========================================================================
// 1. Construction
// ==========================================================================

TEST(OptionalTest, DefaultConstruction) {
    Optional<int> opt;
    EXPECT_FALSE(opt.HasValue());
    EXPECT_FALSE(static_cast<bool>(opt));
}

TEST(OptionalTest, ValueConstruction) {
    Optional<int> opt(42);
    EXPECT_TRUE(opt.HasValue());
    EXPECT_EQ(*opt, 42);
}

TEST(OptionalTest, MoveConstruction) {
    Optional<std::string> opt(std::string("hello world"));
    EXPECT_TRUE(opt.HasValue());
    EXPECT_EQ(*opt, "hello world");
}

TEST(OptionalTest, CopyConstruction) {
    Optional<int> a(42);
    Optional<int> b(a);
    EXPECT_TRUE(b.HasValue());
    EXPECT_EQ(*b, 42);
    EXPECT_TRUE(a.HasValue());
    EXPECT_EQ(*a, 42);
}

// ==========================================================================
// 2. Accessors
// ==========================================================================

TEST(OptionalTest, OperatorBool) {
    Optional<int> empty;
    Optional<int> full(1);
    EXPECT_FALSE(static_cast<bool>(empty));
    EXPECT_TRUE(static_cast<bool>(full));
    if (full) {
        EXPECT_EQ(*full, 1);
    } else {
        FAIL() << "Expected full optional to be true";
    }
}

TEST(OptionalTest, OperatorStar) {
    Optional<int> opt(10);
    *opt = 20;
    EXPECT_EQ(*opt, 20);

    const Optional<int>& const_opt = opt;
    EXPECT_EQ(*const_opt, 20);
}

TEST(OptionalTest, OperatorArrow) {
    Optional<std::string> opt(std::string("hello"));
    EXPECT_EQ(opt->size(), 5u);

    const Optional<std::string>& const_opt = opt;
    EXPECT_EQ(const_opt->size(), 5u);
}

// ==========================================================================
// 3. Assignment
// ==========================================================================

TEST(OptionalTest, ValueAssignment) {
    Optional<int> opt;
    EXPECT_FALSE(opt.HasValue());
    opt = 42;
    EXPECT_TRUE(opt.HasValue());
    EXPECT_EQ(*opt, 42);
}

TEST(OptionalTest, MoveAssignment) {
    Optional<std::string> opt;
    opt = std::string("moved");
    EXPECT_TRUE(opt.HasValue());
    EXPECT_EQ(*opt, "moved");
}

TEST(OptionalTest, ResetByDefaultAssignment) {
    Optional<int> opt(42);
    EXPECT_TRUE(opt.HasValue());
    opt = Optional<int>{};
    EXPECT_FALSE(opt.HasValue());
}

TEST(OptionalTest, OverwriteValue) {
    Optional<int> opt(1);
    EXPECT_EQ(*opt, 1);
    opt = 2;
    EXPECT_EQ(*opt, 2);
    EXPECT_TRUE(opt.HasValue());
}

// ==========================================================================
// 4. Copy-assign Optional-to-Optional
// ==========================================================================

TEST(OptionalTest, CopyAssignFromFull) {
    Optional<int> a(42);
    Optional<int> b;
    b = a;
    EXPECT_TRUE(b.HasValue());
    EXPECT_EQ(*b, 42);
    // Original unchanged.
    EXPECT_TRUE(a.HasValue());
    EXPECT_EQ(*a, 42);
}

TEST(OptionalTest, CopyAssignFromEmpty) {
    Optional<int> a(42);
    Optional<int> b;
    a = b;
    EXPECT_FALSE(a.HasValue());
}

// ==========================================================================
// 5. Complex types
// ==========================================================================

TEST(OptionalTest, WithStruct) {
    llm::ResponseFormat fmt;
    fmt.name = "json_output";
    fmt.schema = nlohmann::json{{"type", "object"}};
    fmt.description = "JSON output format";

    Optional<llm::ResponseFormat> opt(fmt);
    EXPECT_TRUE(opt.HasValue());
    EXPECT_EQ(opt->name, "json_output");
    EXPECT_EQ(opt->description, "JSON output format");
}

TEST(OptionalTest, WithSharedPtr) {
    auto ptr = std::make_shared<int>(99);
    Optional<std::shared_ptr<int>> opt(ptr);
    EXPECT_TRUE(opt.HasValue());
    EXPECT_EQ(**opt, 99);
}
