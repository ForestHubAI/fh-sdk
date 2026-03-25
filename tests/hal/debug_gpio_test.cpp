// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "platform/pc/debug/gpio.hpp"

#include <gtest/gtest.h>

#include <map>

namespace foresthub {
namespace platform {
namespace debug {
namespace {

TEST(DebugGpioTest, DefaultReadReturnsZero) {
    DebugGpio gpio;
    EXPECT_EQ(gpio.DigitalRead(99), 0);
}

TEST(DebugGpioTest, PreLoadedValuesReadBack) {
    std::map<PinID, int> initial = {{4, 1}, {5, 0}, {10, 512}};
    DebugGpio gpio(initial);
    EXPECT_EQ(gpio.DigitalRead(4), 1);
    EXPECT_EQ(gpio.DigitalRead(5), 0);
    EXPECT_EQ(gpio.AnalogRead(10), 512);
}

TEST(DebugGpioTest, DigitalWriteStoresValue) {
    DebugGpio gpio;
    gpio.DigitalWrite(13, 1);
    EXPECT_EQ(gpio.DigitalRead(13), 1);
    gpio.DigitalWrite(13, 0);
    EXPECT_EQ(gpio.DigitalRead(13), 0);
}

TEST(DebugGpioTest, AnalogReadDefaultIsZero) {
    DebugGpio gpio;
    EXPECT_EQ(gpio.AnalogRead(42), 0);
}

TEST(DebugGpioTest, MultiplePinsIndependent) {
    DebugGpio gpio;
    gpio.DigitalWrite(1, 1);
    gpio.DigitalWrite(2, 0);
    EXPECT_EQ(gpio.DigitalRead(1), 1);
    EXPECT_EQ(gpio.DigitalRead(2), 0);
}

TEST(DebugGpioTest, SetPinModeDoesNotCrash) {
    DebugGpio gpio;
    gpio.SetPinMode(0, PinMode::kInput);
    gpio.SetPinMode(1, PinMode::kOutput);
    gpio.SetPinMode(2, PinMode::kInputPullup);
}

TEST(DebugGpioTest, ConfigurePwmReturnsEmpty) {
    DebugGpio gpio;
    PwmConfig config;
    EXPECT_TRUE(gpio.ConfigurePwm(5, config).empty());
}

TEST(DebugGpioTest, PwmWriteDoesNotCrash) {
    DebugGpio gpio;
    gpio.PwmWrite(5, 128);
}

}  // namespace
}  // namespace debug
}  // namespace platform
}  // namespace foresthub
