// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PLATFORM_ARDUINO_GPIO_HPP
#define FORESTHUB_PLATFORM_ARDUINO_GPIO_HPP

#ifdef FORESTHUB_ENABLE_GPIO

#include <array>

#include "foresthub/hal/gpio.hpp"

namespace foresthub {
namespace hal {
namespace arduino {

/// Arduino GPIO using platform-specific digital, analog, and PWM peripherals.
class ArduinoGpio : public Gpio {
public:
    /// Maps PinMode enum to Arduino constant and applies it.
    void SetPinMode(PinID pin, PinMode mode) override;
    /// Drives pin to the given logic level.
    void DigitalWrite(PinID pin, int value) override;
    /// Reads logic level from pin (0 = low, 1 = high).
    int DigitalRead(PinID pin) const override;
    /// Reads raw ADC value from analog pin.
    int AnalogRead(PinID pin) const override;
    /// Configures PWM peripheral (LEDC on ESP32, analogWrite on Portenta H7).
    std::string ConfigurePwm(PinID pin, const PwmConfig& config) override;
    /// Writes duty cycle to a configured PWM pin.
    void PwmWrite(PinID pin, int duty) override;

private:
    static constexpr int kMaxPwmChannels = 16;
    struct PwmEntry {
        PinID pin;
        uint8_t channel;
    };
    std::array<PwmEntry, kMaxPwmChannels> pwm_entries_{};
    uint8_t pwm_count_ = 0;
};

}  // namespace arduino
}  // namespace hal
}  // namespace foresthub

#endif  // FORESTHUB_ENABLE_GPIO

#endif  // FORESTHUB_PLATFORM_ARDUINO_GPIO_HPP
