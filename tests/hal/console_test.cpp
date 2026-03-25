// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "foresthub/hal/platform.hpp"
#include "hal/pc/platform.hpp"

namespace foresthub {
namespace hal {
namespace {

// Helper to get a console instance for testing.
std::shared_ptr<Console> MakeConsole() {
    auto ctx = std::make_shared<pc::PcPlatform>();
    return std::shared_ptr<Console>(ctx, ctx->console.get());
}

// --- TryReadLine ---

TEST(ConsoleTest, TryReadLineNoInputReturnsEmpty) {
    auto console = MakeConsole();
    // With no pending stdin data, TryReadLine should return an empty Optional.
    util::Optional<std::string> result = console->TryReadLine();
    EXPECT_FALSE(result.HasValue());
}

TEST(ConsoleTest, TryReadLineReturnsOptionalType) {
    auto console = MakeConsole();
    util::Optional<std::string> result = console->TryReadLine(128, false);
    // Verify the return type is Optional<string> and has correct default state.
    EXPECT_FALSE(result.has_value);
}

// --- ClearLineBuffer ---

TEST(ConsoleTest, ClearLineBufferDoesNotCrash) {
    auto console = MakeConsole();
    // ClearLineBuffer on empty buffer should not crash.
    console->ClearLineBuffer();
}

TEST(ConsoleTest, ClearLineBufferMultipleCalls) {
    auto console = MakeConsole();
    // Multiple ClearLineBuffer calls should be safe.
    console->ClearLineBuffer();
    console->ClearLineBuffer();
    console->ClearLineBuffer();
}

// --- Available ---

TEST(ConsoleTest, AvailableReturnsBool) {
    auto console = MakeConsole();
    // Available() should return without blocking. In test environment (no TTY input),
    // it returns false. The important thing is it doesn't block.
    bool avail = console->Available();
    (void)avail;  // Result depends on stdin state; just verify no crash/block.
}

}  // namespace
}  // namespace hal
}  // namespace foresthub
