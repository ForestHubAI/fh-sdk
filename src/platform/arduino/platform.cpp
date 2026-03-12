// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifdef FORESTHUB_PLATFORM_ARDUINO  // Translation Unit Guard: Only compile on Arduino

#include "platform.hpp"

#include "console.hpp"
#include "time.hpp"

#ifdef FORESTHUB_ENABLE_NETWORK
#include "http_client.hpp"
#include "network.hpp"
#endif

#ifdef FORESTHUB_ENABLE_CRYPTO
#include "crypto.hpp"
#endif

#ifdef FORESTHUB_ENABLE_GPIO
#include "gpio.hpp"
#endif

namespace foresthub {
namespace platform {
namespace arduino {

ArduinoPlatformContext::ArduinoPlatformContext(const PlatformConfig& config) {
    console = std::make_shared<ArduinoConsole>(config.baud_rate);
    time = std::make_shared<ArduinoTime>();

#ifdef FORESTHUB_ENABLE_NETWORK
    network = std::make_shared<ArduinoNetwork>(config.network);
#endif

#ifdef FORESTHUB_ENABLE_CRYPTO
    crypto = std::make_shared<ArduinoCrypto>();
#endif

#ifdef FORESTHUB_ENABLE_GPIO
    gpio = std::make_shared<ArduinoGpio>();
#endif
}

std::shared_ptr<core::HttpClient> ArduinoPlatformContext::CreateHttpClient(const HttpClientConfig& config) {
#ifdef FORESTHUB_ENABLE_NETWORK
    if (config.use_tls) {
#ifdef FORESTHUB_ENABLE_CRYPTO
        // TLS requires crypto subsystem for certificate management
        auto* arduino_crypto = static_cast<ArduinoCrypto*>(crypto.get());
        std::shared_ptr<TLSClientWrapper> tls_wrapper = arduino_crypto->CreateTlsClient(nullptr, config.timeout_ms);
        return std::make_shared<ArduinoHttpClientWrapper>(std::move(tls_wrapper), config.host, config.port,
                                                          config.timeout_ms);
#else
        // TLS requested but FORESTHUB_ENABLE_CRYPTO not defined
        return nullptr;
#endif
    }

    // Plain HTTP — WiFiClient needs only NETWORK, not CRYPTO
    auto plain_client = std::make_unique<WiFiClient>();
    plain_client->setTimeout(config.timeout_ms / 1000);
    return std::make_shared<ArduinoHttpClientWrapper>(std::move(plain_client), config.host, config.port,
                                                      config.timeout_ms);
#else
    (void)config;
    return nullptr;
#endif
}

}  // namespace arduino
}  // namespace platform
}  // namespace foresthub

// Factory function (called by application layer)
namespace foresthub {
namespace platform {

std::shared_ptr<PlatformContext> CreatePlatform(const PlatformConfig& config) {
    return std::make_shared<arduino::ArduinoPlatformContext>(config);
}

}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_ARDUINO
