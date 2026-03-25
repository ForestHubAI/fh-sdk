// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef APP_PLATFORM_SETUP_HPP
#define APP_PLATFORM_SETUP_HPP

#include <cstdio>
#include <cstdlib>
#include <memory>

#include "foresthub/hal/platform.hpp"
#include "hal/pc/platform.hpp"

namespace app {

// Exit codes for platform initialization failures.
// Applications should use these for consistent error reporting.
constexpr int kExitPlatformFailed = 10;
constexpr int kExitConsoleFailed = 11;
constexpr int kExitNetworkFailed = 12;
constexpr int kExitTimeFailed = 13;

// Initialize the platform and all subsystems.
// Returns a fully initialized Platform, or exits on failure.
// This function never returns an invalid platform -- it exits the process if initialization fails.
inline std::shared_ptr<foresthub::hal::Platform> SetupPlatform() {
    // 1. Create platform context
    auto platform = std::make_shared<foresthub::hal::pc::PcPlatform>();
    if (!platform) {
        // Must use fprintf(stderr) here because console is not yet initialized
        std::fprintf(stderr, "[FATAL] Failed to create platform context\n");
        std::exit(kExitPlatformFailed);
    }

    // 2. Initialize console (void return, no error check needed)
    platform->console->Begin();

    // 3. Connect network
    std::string net_err = platform->network->Connect();
    if (!net_err.empty()) {
        platform->console->Printf("[FATAL] Failed to connect network: %s\n", net_err.c_str());
        platform->console->Flush();
        std::exit(kExitNetworkFailed);
    }

    // 4. Synchronize time
    std::string time_err = platform->time->SyncTime();
    if (!time_err.empty()) {
        platform->console->Printf("[FATAL] Failed to sync time: %s\n", time_err.c_str());
        platform->console->Flush();
        std::exit(kExitTimeFailed);
    }

    return platform;
}

}  // namespace app

#endif  // APP_PLATFORM_SETUP_HPP
