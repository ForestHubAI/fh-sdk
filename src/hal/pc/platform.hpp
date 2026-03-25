// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PLATFORM_PC_PLATFORM_HPP
#define FORESTHUB_PLATFORM_PC_PLATFORM_HPP

#include "foresthub/hal/platform.hpp"

namespace foresthub {
namespace hal {
namespace pc {

/// Construction-time configuration for the PC platform.
struct PcConfig {};

/// PC platform context. Creates all HAL subsystems in the constructor.
class PcPlatform : public Platform {
public:
    /// Initialize PC platform with all subsystems.
    explicit PcPlatform(const PcConfig& config = {});

    /// Ignores host/port/use_tls; CPR handles routing per request.
    std::shared_ptr<HttpClient> CreateHttpClient(const HttpClientConfig& config) override;
};

}  // namespace pc
}  // namespace hal
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_PC_PLATFORM_HPP
