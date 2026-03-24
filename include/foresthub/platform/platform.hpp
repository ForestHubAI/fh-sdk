// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PLATFORM_PLATFORM_HPP
#define FORESTHUB_PLATFORM_PLATFORM_HPP

/// @file
/// Platform base class and subsystem aggregation.

#include <memory>

#include "foresthub/platform/platform_config.hpp"

#include "foresthub/core/http_client.hpp"
#include "foresthub/platform/console.hpp"
#include "foresthub/platform/crypto.hpp"
#include "foresthub/platform/gpio.hpp"
#include "foresthub/platform/network.hpp"
#include "foresthub/platform/time.hpp"

namespace foresthub {
namespace platform {

/// Holds all platform subsystems and provides a factory for HTTP clients.
///
/// Each platform defines its own concrete subclass with a platform-specific
/// config struct. Application code constructs the platform directly:
///   auto p = std::make_shared<pc::PcPlatform>(config);
///   auto p = std::make_shared<arduino::ArduinoPlatform>(config);
///   auto p = std::make_shared<debug::DebugPlatform>(config);
class Platform {
public:
    std::shared_ptr<Network> network;  ///< Network connectivity subsystem.
    std::shared_ptr<Console> console;  ///< Console I/O subsystem.
    std::shared_ptr<Time> time;        ///< Time and delay subsystem.
    std::shared_ptr<Crypto> crypto;    ///< TLS/crypto subsystem.
    std::shared_ptr<Gpio> gpio;  ///< GPIO pin I/O subsystem (nullptr if FORESTHUB_ENABLE_GPIO not defined).

    /// Create a platform-specific HTTP client.
    /// @param config Connection parameters (host, port, TLS, timeout).
    /// @return HTTP client configured for the given parameters.
    virtual std::shared_ptr<core::HttpClient> CreateHttpClient(const core::HttpClientConfig& config) = 0;

    virtual ~Platform() = default;
};

}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_PLATFORM_HPP
