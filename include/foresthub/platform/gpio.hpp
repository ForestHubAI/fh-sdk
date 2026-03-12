// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PLATFORM_GPIO_HPP
#define FORESTHUB_PLATFORM_GPIO_HPP

/// @file
/// Abstract interface for general-purpose pin I/O and PWM.

#include <cstdint>
#include <string>

namespace foresthub {
namespace platform {

/// 32-bit pin identifier.
///
/// Wide enough for port+pin encoding schemes (e.g. port << 8 | pin) used by
/// various microcontroller families while remaining trivially copyable.
using PinId = uint32_t;

/// Direction and pull configuration of a pin.
///
/// Values are platform-independent; implementations map them to
/// hardware-specific constants at runtime.
enum class PinMode : uint8_t {
    kInput = 0,        ///< High-impedance input (floating).
    kOutput = 1,       ///< Push-pull output.
    kInputPullup = 2,  ///< Input with internal pull-up resistor enabled.
};

/// Configuration for a PWM output channel.
struct PwmConfig {
    uint32_t frequency_hz = 1000;  ///< PWM frequency in hertz.
    uint8_t resolution_bits = 8;   ///< Duty-cycle resolution in bits (e.g. 8 = 0..255).
};

/// Abstract interface for general-purpose pin I/O.
///
/// Covers digital input/output, analog-to-digital conversion, and
/// pulse-width modulation. Platform implementations translate calls
/// to the underlying peripheral hardware.
class GpioInterface {
public:
    virtual ~GpioInterface() = default;

    /// Configure a pin's direction and pull mode.
    /// @param pin Target pin identifier.
    /// @param mode Desired direction and pull configuration.
    virtual void SetPinMode(PinId pin, PinMode mode) = 0;

    /// Set a digital output pin to the given logic level.
    /// @param pin Target pin identifier (must be configured as output).
    /// @param value Logic level to drive (0 = low, 1 = high).
    virtual void DigitalWrite(PinId pin, int value) = 0;

    /// Read the current logic level of a digital input pin.
    /// @param pin Target pin identifier.
    /// @return 0 for low, 1 for high.
    virtual int DigitalRead(PinId pin) const = 0;

    /// Read the raw ADC value from an analog input pin.
    /// @param pin Target pin identifier (must support analog input).
    /// @return Raw ADC reading; range depends on hardware resolution.
    virtual int AnalogRead(PinId pin) const = 0;

    /// Attach and configure a PWM peripheral on the given pin.
    /// @param pin Target pin identifier (must support PWM output).
    /// @param config Frequency and resolution settings.
    /// @return Empty string on success, error message on failure.
    virtual std::string ConfigurePwm(PinId pin, const PwmConfig& config) = 0;

    /// Set the duty cycle on a previously configured PWM pin.
    /// @param pin Target pin identifier (must have been configured via ConfigurePwm).
    /// @param duty Duty-cycle value in the range [0, 2^resolution_bits - 1].
    virtual void PwmWrite(PinId pin, int duty) = 0;
};

}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_GPIO_HPP
