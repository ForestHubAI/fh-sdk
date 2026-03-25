// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PLATFORM_ARDUINO_PLATFORM_HPP
#define FORESTHUB_PLATFORM_ARDUINO_PLATFORM_HPP

#include "foresthub/hal/network.hpp"
#include "foresthub/hal/platform.hpp"

namespace foresthub {
namespace hal {
namespace arduino {

/// Construction-time configuration for the Arduino platform.
struct ArduinoConfig {
    NetworkConfig network;             ///< WiFi credentials.
    unsigned long baud_rate = 115200;  ///< Serial communication speed in bits per second.
};

/// Arduino platform context aggregating Network, Console, Time, and Crypto subsystems.
class ArduinoPlatform : public Platform {
public:
    /// Constructs all HAL subsystems from the given platform configuration.
    explicit ArduinoPlatform(const ArduinoConfig& config);

    /// Configures TLS or plain connections based on config.use_tls flag.
    std::shared_ptr<HttpClient> CreateHttpClient(const HttpClientConfig& config) override;
};

}  // namespace arduino
}  // namespace hal
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_ARDUINO_PLATFORM_HPP
