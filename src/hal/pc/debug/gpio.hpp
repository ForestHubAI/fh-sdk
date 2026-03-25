// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PLATFORM_DEBUG_GPIO_HPP
#define FORESTHUB_PLATFORM_DEBUG_GPIO_HPP

#include <map>

#include "foresthub/hal/gpio.hpp"

namespace foresthub {
namespace hal {
namespace debug {

/// Debug GPIO backed by an in-memory map.
/// Reads and writes log to stdout with "console: GPIO: " prefix for the Go backend protocol.
class DebugGpio : public Gpio {
public:
    /// @param initial_values Pin values to pre-load (pin → value).
    explicit DebugGpio(const std::map<PinID, int>& initial_values = {});

    void SetPinMode(PinID pin, PinMode mode) override;
    void DigitalWrite(PinID pin, int value) override;
    int DigitalRead(PinID pin) const override;
    int AnalogRead(PinID pin) const override;
    std::string ConfigurePwm(PinID pin, const PwmConfig& config) override;
    void PwmWrite(PinID pin, int duty) override;

private:
    std::map<PinID, int> pin_values_;
};

}  // namespace debug
}  // namespace hal
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_DEBUG_GPIO_HPP
