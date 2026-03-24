// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#if defined(FORESTHUB_PLATFORM_ARDUINO) && defined(FORESTHUB_ENABLE_GPIO)

#include "gpio.hpp"

#include <Arduino.h>

namespace foresthub {
namespace platform {
namespace arduino {

// Enum mapping is inlined in each method rather than via helper functions because
// ESP32 uses uint8_t for pin mode/value, while Portenta H7 Mbed uses typed enums
// (::PinMode, ::PinStatus). Passing Arduino constants directly avoids type mismatches.

void ArduinoGpio::SetPinMode(PinID pin, PinMode mode) {
    uint8_t hw_pin = static_cast<uint8_t>(pin);
    switch (mode) {
        case PinMode::kInput:
            ::pinMode(hw_pin, INPUT);
            return;
        case PinMode::kOutput:
            ::pinMode(hw_pin, OUTPUT);
            return;
        case PinMode::kInputPullup:
            ::pinMode(hw_pin, INPUT_PULLUP);
            return;
    }
}

void ArduinoGpio::DigitalWrite(PinID pin, int value) {
    ::digitalWrite(static_cast<uint8_t>(pin), value ? HIGH : LOW);
}

int ArduinoGpio::DigitalRead(PinID pin) const {
    return (::digitalRead(static_cast<uint8_t>(pin)) == HIGH) ? 1 : 0;
}

int ArduinoGpio::AnalogRead(PinID pin) const {
    return ::analogRead(static_cast<uint8_t>(pin));
}

#if defined(ARDUINO_ARCH_ESP32)

// ESP32: LEDC peripheral for hardware PWM (Arduino Core v2.x API).
// Each pin requires a dedicated LEDC channel (0-15).

std::string ArduinoGpio::ConfigurePwm(PinID pin, const PwmConfig& config) {
    if (pwm_count_ >= kMaxPwmChannels) {
        return "No free LEDC channels available";
    }

    uint8_t channel = pwm_count_;
    ledcSetup(channel, config.frequency_hz, config.resolution_bits);
    ledcAttachPin(static_cast<uint8_t>(pin), channel);

    pwm_entries_[pwm_count_].pin = pin;
    pwm_entries_[pwm_count_].channel = channel;
    ++pwm_count_;

    return "";
}

void ArduinoGpio::PwmWrite(PinID pin, int duty) {
    // Linear scan for pin-to-channel mapping (typically < 4 PWM pins)
    for (uint8_t i = 0; i < pwm_count_; ++i) {
        if (pwm_entries_[i].pin == pin) {
            ledcWrite(pwm_entries_[i].channel, static_cast<uint32_t>(duty));
            return;
        }
    }
}

#elif defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_PORTENTA_H7_M4)

// Portenta H7: analogWrite with configurable resolution.
// Frequency is best-effort (hardware-determined).

std::string ArduinoGpio::ConfigurePwm(PinID pin, const PwmConfig& config) {
    analogWriteResolution(config.resolution_bits);
    return "";
}

void ArduinoGpio::PwmWrite(PinID pin, int duty) {
    ::analogWrite(static_cast<uint8_t>(pin), duty);
}

#endif

}  // namespace arduino
}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_ARDUINO && FORESTHUB_ENABLE_GPIO
