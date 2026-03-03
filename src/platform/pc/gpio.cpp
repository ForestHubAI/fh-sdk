#ifdef FORESTHUB_PLATFORM_PC  // Translation Unit Guard: Only compile on PC

#include "gpio.hpp"

namespace foresthub {
namespace platform {
namespace pc {

void PcGpio::SetPinMode(PinId pin, PinMode mode) {
    pin_modes_[pin] = mode;
}

void PcGpio::DigitalWrite(PinId pin, int value) {
    pin_values_[pin] = value;
}

int PcGpio::DigitalRead(PinId pin) const {
    auto it = pin_values_.find(pin);
    if (it != pin_values_.end()) {
        return it->second;
    }
    return 0;
}

int PcGpio::AnalogRead(PinId pin) const {
    auto it = pin_values_.find(pin);
    if (it != pin_values_.end()) {
        return it->second;
    }
    return 0;
}

std::string PcGpio::ConfigurePwm(PinId pin, const PwmConfig& config) {
    pwm_configs_[pin] = config;
    return "";
}

void PcGpio::PwmWrite(PinId pin, int duty) {
    pwm_duties_[pin] = duty;
}

}  // namespace pc
}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_PC
