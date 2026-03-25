// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "gpio.hpp"

namespace foresthub {
namespace hal {
namespace pc {

void PcGpio::SetPinMode(PinID pin, PinMode mode) {
    pin_modes_[pin] = mode;
}

void PcGpio::DigitalWrite(PinID pin, int value) {
    pin_values_[pin] = value;
}

int PcGpio::DigitalRead(PinID pin) const {
    auto it = pin_values_.find(pin);
    if (it != pin_values_.end()) {
        return it->second;
    }
    return 0;
}

int PcGpio::AnalogRead(PinID pin) const {
    auto it = pin_values_.find(pin);
    if (it != pin_values_.end()) {
        return it->second;
    }
    return 0;
}

std::string PcGpio::ConfigurePwm(PinID pin, const PwmConfig& config) {
    pwm_configs_[pin] = config;
    return "";
}

void PcGpio::PwmWrite(PinID pin, int duty) {
    pwm_duties_[pin] = duty;
}

}  // namespace pc
}  // namespace hal
}  // namespace foresthub
