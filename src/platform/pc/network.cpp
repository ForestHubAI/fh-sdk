// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "network.hpp"

namespace foresthub {
namespace platform {
namespace pc {

std::string PcNetwork::Connect(unsigned long /*timeout_ms*/) {
    // PC networking is handled by the OS. Always succeeds.
    return "";
}

void PcNetwork::Disconnect() {
    // No-op on PC.
}

NetworkStatus PcNetwork::GetStatus() const {
    return NetworkStatus::kConnected;
}

std::string PcNetwork::GetLocalIp() const {
    return "127.0.0.1";
}

int PcNetwork::GetSignalStrength() const {
    return 0;  // Not applicable on PC.
}

}  // namespace pc
}  // namespace platform
}  // namespace foresthub
