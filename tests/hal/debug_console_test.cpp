// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hal/pc/debug/console.hpp"

namespace foresthub {
namespace hal {
namespace debug {
namespace {

TEST(DebugConsoleTest, EmptyQueueReadLineReturnsEmpty) {
    DebugConsole console;
    EXPECT_EQ(console.ReadLine(), "");
}

TEST(DebugConsoleTest, EmptyQueueTryReadLineReturnsEmpty) {
    DebugConsole console;
    auto result = console.TryReadLine();
    EXPECT_FALSE(result.HasValue());
}

TEST(DebugConsoleTest, EmptyQueueAvailableFalse) {
    DebugConsole console;
    EXPECT_FALSE(console.Available());
}

TEST(DebugConsoleTest, PreLoadedLinesReadInFifoOrder) {
    DebugConsole console({"first", "second", "third"});
    EXPECT_TRUE(console.Available());
    EXPECT_EQ(console.ReadLine(), "first");
    EXPECT_EQ(console.ReadLine(), "second");
    EXPECT_EQ(console.ReadLine(), "third");
    EXPECT_FALSE(console.Available());
}

TEST(DebugConsoleTest, TryReadLineReturnsPreLoadedLines) {
    DebugConsole console({"hello"});
    auto result = console.TryReadLine();
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(*result, "hello");
}

TEST(DebugConsoleTest, ReadLineTruncatesAtMaxLength) {
    DebugConsole console({"long_input_string"});
    std::string line = console.ReadLine(4);
    EXPECT_EQ(line, "long");
}

TEST(DebugConsoleTest, ReadCharacterByCharacter) {
    DebugConsole console({"ab"});
    EXPECT_EQ(console.Read(), 'a');
    EXPECT_EQ(console.Read(), 'b');
    EXPECT_EQ(console.Read(), -1);
}

TEST(DebugConsoleTest, BeginAndFlushDoNotCrash) {
    DebugConsole console;
    console.Begin();
    console.Flush();
}

TEST(DebugConsoleTest, ClearLineBufferDoesNotCrash) {
    DebugConsole console({"data"});
    console.ClearLineBuffer();
    // Queue should still be intact (ClearLineBuffer is no-op for debug).
    EXPECT_TRUE(console.Available());
}

}  // namespace
}  // namespace debug
}  // namespace hal
}  // namespace foresthub
