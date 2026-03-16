// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "foresthub/util/strprintf.hpp"

#include <gtest/gtest.h>

#include <string>

using foresthub::util::StrPrintf;

// ==========================================================================
// 1. Basic Formatting
// ==========================================================================

TEST(StrPrintfTest, EmptyFormatString) {
    EXPECT_EQ(StrPrintf(""), "");
}

TEST(StrPrintfTest, PlainTextNoArgs) {
    EXPECT_EQ(StrPrintf("hello"), "hello");
}

TEST(StrPrintfTest, StringArg) {
    EXPECT_EQ(StrPrintf("name: %s", "Alice"), "name: Alice");
}

TEST(StrPrintfTest, IntArg) {
    EXPECT_EQ(StrPrintf("count: %d", 42), "count: 42");
}

TEST(StrPrintfTest, FloatArg) {
    std::string result = StrPrintf("temp: %f", 3.14);
    EXPECT_EQ(result.substr(0, 10), "temp: 3.14");
}

TEST(StrPrintfTest, MixedArgs) {
    EXPECT_EQ(StrPrintf("%s = %d", "x", 5), "x = 5");
}

TEST(StrPrintfTest, PercentEscaping) {
    EXPECT_EQ(StrPrintf("100%%"), "100%");
}

// ==========================================================================
// 2. Type-Specific Formatting
// ==========================================================================

TEST(StrPrintfTest, NegativeInt) {
    EXPECT_EQ(StrPrintf("n: %d", -42), "n: -42");
}

TEST(StrPrintfTest, BoolAsInt) {
    EXPECT_EQ(StrPrintf("flag: %d", static_cast<int>(true)), "flag: 1");
    EXPECT_EQ(StrPrintf("flag: %d", static_cast<int>(false)), "flag: 0");
}

TEST(StrPrintfTest, FloatPrecision) {
    EXPECT_EQ(StrPrintf("val: %.2f", 3.14159), "val: 3.14");
}

// ==========================================================================
// 3. std::string Integration
// ==========================================================================

TEST(StrPrintfTest, StdStringCStr) {
    std::string name = "Bob";
    EXPECT_EQ(StrPrintf("hello %s", name.c_str()), "hello Bob");
}

TEST(StrPrintfTest, MultipleStringArgs) {
    std::string a = "foo";
    std::string b = "bar";
    EXPECT_EQ(StrPrintf("%s und Kontext: %s", a.c_str(), b.c_str()), "foo und Kontext: bar");
}

// ==========================================================================
// 4. Large Strings
// ==========================================================================

TEST(StrPrintfTest, LargeString) {
    std::string payload(2048, 'X');
    std::string result = StrPrintf("data: %s", payload.c_str());
    EXPECT_EQ(result.size(), 6 + 2048);  // "data: " + payload
    EXPECT_EQ(result.substr(0, 6), "data: ");
    EXPECT_EQ(result.back(), 'X');
}

TEST(StrPrintfTest, LargeFormatResult) {
    // Simulate RAG-style output >4KB
    std::string ctx(4096, 'A');
    std::string serial(1024, 'B');
    std::string result = StrPrintf("%s und Kontext: %s", serial.c_str(), ctx.c_str());
    EXPECT_EQ(result.size(), 1024 + 14 + 4096);  // serial + " und Kontext: " + ctx
}
