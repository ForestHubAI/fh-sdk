// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PLATFORM_PC_GPIO_HPP
#define FORESTHUB_PLATFORM_PC_GPIO_HPP

#include <map>

#include "foresthub/hal/gpio.hpp"

namespace foresthub {
namespace hal {
namespace pc {

/// In-memory GPIO mock for PC. Stores pin state in maps for host-based testing.
class PcGpio : public Gpio {
public:
    /// Stores the pin mode.
    void SetPinMode(PinID pin, PinMode mode) override;
    /// Stores the value for later read-back.
    void DigitalWrite(PinID pin, int value) override;
    /// Returns the last written value, or 0 if never written.
    int DigitalRead(PinID pin) const override;
    /// Returns the last written value as int, or 0 if never written.
    int AnalogRead(PinID pin) const override;
    /// Stores the config. Always succeeds.
    std::string ConfigurePwm(PinID pin, const PwmConfig& config) override;
    /// Stores the duty cycle value.
    void PwmWrite(PinID pin, int duty) override;

private:
    std::map<PinID, PinMode> pin_modes_;
    std::map<PinID, int> pin_values_;
    std::map<PinID, PwmConfig> pwm_configs_;
    std::map<PinID, int> pwm_duties_;
};

}  // namespace pc
}  // namespace hal
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_PC_GPIO_HPP
