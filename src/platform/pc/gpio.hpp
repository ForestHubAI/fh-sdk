#ifndef FORESTHUB_PLATFORM_PC_GPIO_HPP
#define FORESTHUB_PLATFORM_PC_GPIO_HPP

#include <map>

#include "foresthub/platform/gpio.hpp"

namespace foresthub {
namespace platform {
namespace pc {

/// In-memory GPIO mock for PC. Stores pin state in maps for host-based testing.
class PcGpio : public GpioInterface {
public:
    /// Stores the pin mode.
    void SetPinMode(PinId pin, PinMode mode) override;
    /// Stores the value for later read-back.
    void DigitalWrite(PinId pin, int value) override;
    /// Returns the last written value, or 0 if never written.
    int DigitalRead(PinId pin) const override;
    /// Returns the last written value as int, or 0 if never written.
    int AnalogRead(PinId pin) const override;
    /// Stores the config. Always succeeds.
    std::string ConfigurePwm(PinId pin, const PwmConfig& config) override;
    /// Stores the duty cycle value.
    void PwmWrite(PinId pin, int duty) override;

private:
    std::map<PinId, PinMode> pin_modes_;
    std::map<PinId, int> pin_values_;
    std::map<PinId, PwmConfig> pwm_configs_;
    std::map<PinId, int> pwm_duties_;
};

}  // namespace pc
}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_PC_GPIO_HPP
