// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "platform.hpp"

#include "console.hpp"
#include "crypto.hpp"
#include "gpio.hpp"
#include "http_client.hpp"
#include "network.hpp"
#include "time.hpp"

namespace foresthub {
namespace platform {
namespace pc {

PcPlatform::PcPlatform(const PcConfig& /*config*/) {
    network = std::make_shared<PcNetwork>();
    console = std::make_shared<PcConsole>();
    time = std::make_shared<PcTime>();
    crypto = std::make_shared<PcCrypto>();
    gpio = std::make_shared<PcGpio>();
}

std::shared_ptr<core::HttpClient> PcPlatform::CreateHttpClient(const core::HttpClientConfig& config) {
    // PcHttpClient uses CPR/libcurl which handles host, port, and TLS internally per-request.
    // Only timeout is relevant at construction time.
    return std::make_shared<PcHttpClient>(static_cast<int>(config.timeout_ms));
}

}  // namespace pc
}  // namespace platform
}  // namespace foresthub
