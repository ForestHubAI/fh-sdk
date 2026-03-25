// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "platform.hpp"

#include "console.hpp"
#include "gpio.hpp"
#include "platform/pc/crypto.hpp"
#include "platform/pc/http_client.hpp"
#include "platform/pc/network.hpp"
#include "platform/pc/time.hpp"

namespace foresthub {
namespace platform {
namespace debug {

DebugPlatform::DebugPlatform(const DebugConfig& config) {
    network = std::make_shared<pc::PcNetwork>();
    console = std::make_shared<DebugConsole>(config.serial_lines);
    time = std::make_shared<pc::PcTime>();
    crypto = std::make_shared<pc::PcCrypto>();
    gpio = std::make_shared<DebugGpio>(config.gpio_values);
}

std::shared_ptr<core::HttpClient> DebugPlatform::CreateHttpClient(const core::HttpClientConfig& config) {
    return std::make_shared<pc::PcHttpClient>(static_cast<int>(config.timeout_ms));
}

}  // namespace debug
}  // namespace platform
}  // namespace foresthub
