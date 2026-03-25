// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PLATFORM_PLATFORM_HPP
#define FORESTHUB_PLATFORM_PLATFORM_HPP

/// @file
/// Platform base class and subsystem aggregation.

#include <memory>
#include "foresthub/llm/http_client.hpp"
#include "foresthub/hal/console.hpp"
#include "foresthub/hal/crypto.hpp"
#include "foresthub/hal/gpio.hpp"
#include "foresthub/hal/network.hpp"
#include "foresthub/hal/time.hpp"

namespace foresthub {
namespace hal {

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
    virtual std::shared_ptr<llm::HttpClient> CreateHttpClient(const llm::HttpClientConfig& config) = 0;

    virtual ~Platform() = default;
};

}  // namespace hal
}  // namespace foresthub

// Compile-time validation of platform selection macros.
// Exactly one FORESTHUB_PLATFORM_* must be defined
#if defined(FORESTHUB_PLATFORM_PC) + defined(FORESTHUB_PLATFORM_PC_DEBUG) + defined(FORESTHUB_PLATFORM_ARDUINO) != 1
#error "Exactly one FORESTHUB_PLATFORM_* must be defined (PC, PC_DEBUG, or ARDUINO)"
#endif

#endif  // FORESTHUB_PLATFORM_PLATFORM_HPP
