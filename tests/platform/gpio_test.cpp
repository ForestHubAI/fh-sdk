// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include <gtest/gtest.h>

#include <memory>

#include "foresthub/platform/platform.hpp"

namespace foresthub {
namespace platform {
namespace {

// --- Platform integration ---

TEST(GpioTest, GpioSubsystemNonNullByDefault) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    ASSERT_NE(ctx, nullptr);
    EXPECT_NE(ctx->gpio, nullptr);
}

// --- SetPinMode ---

TEST(GpioTest, SetPinModeDoesNotCrash) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    ctx->gpio->SetPinMode(0, PinMode::kInput);
    ctx->gpio->SetPinMode(1, PinMode::kOutput);
    ctx->gpio->SetPinMode(2, PinMode::kInputPullup);
}

// --- Digital I/O ---

TEST(GpioTest, DigitalWriteReadRoundTrip) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    ctx->gpio->SetPinMode(13, PinMode::kOutput);

    ctx->gpio->DigitalWrite(13, 1);
    EXPECT_EQ(ctx->gpio->DigitalRead(13), 1);

    ctx->gpio->DigitalWrite(13, 0);
    EXPECT_EQ(ctx->gpio->DigitalRead(13), 0);
}

TEST(GpioTest, DigitalReadDefaultIsLow) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    EXPECT_EQ(ctx->gpio->DigitalRead(99), 0);
}

// --- Analog read ---

TEST(GpioTest, AnalogReadDefaultIsZero) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    EXPECT_EQ(ctx->gpio->AnalogRead(42), 0);
}

TEST(GpioTest, AnalogReadAfterDigitalWrite) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    ctx->gpio->DigitalWrite(10, 512);
    EXPECT_EQ(ctx->gpio->AnalogRead(10), 512);
}

// --- PWM ---

TEST(GpioTest, ConfigurePwmSuccess) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    PwmConfig config;
    std::string err = ctx->gpio->ConfigurePwm(5, config);
    EXPECT_TRUE(err.empty());
}

TEST(GpioTest, ConfigurePwmCustomConfig) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    PwmConfig config;
    config.frequency_hz = 50000;
    config.resolution_bits = 10;
    std::string err = ctx->gpio->ConfigurePwm(6, config);
    EXPECT_TRUE(err.empty());
}

TEST(GpioTest, PwmWriteDoesNotCrash) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    PwmConfig config;
    ctx->gpio->ConfigurePwm(5, config);
    ctx->gpio->PwmWrite(5, 128);
}

// --- Edge cases ---

TEST(GpioTest, MultiplePinsIndependent) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    ctx->gpio->DigitalWrite(1, 1);
    ctx->gpio->DigitalWrite(2, 0);

    EXPECT_EQ(ctx->gpio->DigitalRead(1), 1);
    EXPECT_EQ(ctx->gpio->DigitalRead(2), 0);
}

TEST(GpioTest, LargePinIdWorks) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    PinId large_pin = 0xFFFF0001;

    ctx->gpio->SetPinMode(large_pin, PinMode::kOutput);
    ctx->gpio->DigitalWrite(large_pin, 1);
    EXPECT_EQ(ctx->gpio->DigitalRead(large_pin), 1);
}

}  // namespace
}  // namespace platform
}  // namespace foresthub
