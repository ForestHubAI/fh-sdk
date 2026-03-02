#ifndef FORESTHUB_PLATFORM_ARDUINO_PLATFORM_HPP
#define FORESTHUB_PLATFORM_ARDUINO_PLATFORM_HPP

#include "foresthub/platform/platform.hpp"

namespace foresthub {
namespace platform {
namespace arduino {

/// Arduino platform context aggregating Network, Console, Time, and Crypto subsystems.
class ArduinoPlatformContext : public PlatformContext {
public:
    /// Constructs all HAL subsystems from the given platform configuration.
    explicit ArduinoPlatformContext(const PlatformConfig& config);

    /// Configures TLS or plain connections based on config.use_tls flag.
    std::shared_ptr<core::HttpClient> CreateHttpClient(const HttpClientConfig& config) override;
};

}  // namespace arduino
}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_ARDUINO_PLATFORM_HPP
