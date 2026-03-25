// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "gpio.hpp"

#include <iostream>

namespace foresthub {
namespace platform {
namespace debug {

DebugGpio::DebugGpio(const std::map<PinID, int>& initial_values) : pin_values_(initial_values) {}

void DebugGpio::SetPinMode(PinID /*pin*/, PinMode /*mode*/) {
    // No-op for debug platform.
}

void DebugGpio::DigitalWrite(PinID pin, int value) {
    pin_values_[pin] = value;
    std::cout << "console: GPIO: wrote " << value << " to pin " << pin << std::endl;
}

int DebugGpio::DigitalRead(PinID pin) const {
    int value = 0;
    auto it = pin_values_.find(pin);
    if (it != pin_values_.end()) {
        value = it->second;
    }
    std::cout << "console: GPIO: read pin " << pin << " = " << value << std::endl;
    return value;
}

int DebugGpio::AnalogRead(PinID pin) const {
    int value = 0;
    auto it = pin_values_.find(pin);
    if (it != pin_values_.end()) {
        value = it->second;
    }
    std::cout << "console: GPIO: analog read pin " << pin << " = " << value << std::endl;
    return value;
}

std::string DebugGpio::ConfigurePwm(PinID /*pin*/, const PwmConfig& /*config*/) {
    return "";
}

void DebugGpio::PwmWrite(PinID /*pin*/, int /*duty*/) {
    // No-op for debug platform.
}

}  // namespace debug
}  // namespace platform
}  // namespace foresthub
