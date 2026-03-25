// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PLATFORM_DEBUG_PLATFORM_HPP
#define FORESTHUB_PLATFORM_DEBUG_PLATFORM_HPP

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "foresthub/hal/platform.hpp"

namespace foresthub {
namespace hal {
namespace debug {

/// Construction-time configuration for the debug platform.
/// Contains pre-populated GPIO pin values and serial input lines
/// that the Go backend passes via stdin JSON.
struct DebugConfig {
    std::map<uint32_t, int> gpio_values;      ///< Pin → value to pre-load.
    std::vector<std::string> serial_lines;    ///< Lines to pre-load into console input queue.
};

/// Debug platform context for server-side workflow debugging.
///
/// Console output is prefixed with "console: " for the Go backend protocol.
/// GPIO and serial input are pre-populated from DebugConfig.
/// Network, time, crypto, and HTTP reuse the PC implementations.
class DebugPlatform : public Platform {
public:
    explicit DebugPlatform(const DebugConfig& config = {});

    /// Returns PcHttpClient (real HTTP via CPR — agent/RAG calls hit the real backend).
    std::shared_ptr<HttpClient> CreateHttpClient(const HttpClientConfig& config) override;
};

}  // namespace debug
}  // namespace hal
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_DEBUG_PLATFORM_HPP
