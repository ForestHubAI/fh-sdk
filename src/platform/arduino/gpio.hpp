// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PLATFORM_ARDUINO_GPIO_HPP
#define FORESTHUB_PLATFORM_ARDUINO_GPIO_HPP

#ifdef FORESTHUB_ENABLE_GPIO

#include <array>

#include "foresthub/platform/gpio.hpp"

namespace foresthub {
namespace platform {
namespace arduino {

/// Arduino GPIO using platform-specific digital, analog, and PWM peripherals.
class ArduinoGpio : public GpioInterface {
public:
    /// Maps PinMode enum to Arduino constant and applies it.
    void SetPinMode(PinId pin, PinMode mode) override;
    /// Drives pin to the given logic level.
    void DigitalWrite(PinId pin, int value) override;
    /// Reads logic level from pin (0 = low, 1 = high).
    int DigitalRead(PinId pin) const override;
    /// Reads raw ADC value from analog pin.
    int AnalogRead(PinId pin) const override;
    /// Configures PWM peripheral (LEDC on ESP32, analogWrite on Portenta H7).
    std::string ConfigurePwm(PinId pin, const PwmConfig& config) override;
    /// Writes duty cycle to a configured PWM pin.
    void PwmWrite(PinId pin, int duty) override;

private:
    static constexpr int kMaxPwmChannels = 16;
    struct PwmEntry {
        PinId pin;
        uint8_t channel;
    };
    std::array<PwmEntry, kMaxPwmChannels> pwm_entries_{};
    uint8_t pwm_count_ = 0;
};

}  // namespace arduino
}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_ENABLE_GPIO

#endif  // FORESTHUB_PLATFORM_ARDUINO_GPIO_HPP
