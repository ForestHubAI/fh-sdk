// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#if defined(FORESTHUB_PLATFORM_ARDUINO) && defined(FORESTHUB_ENABLE_NETWORK)

#include "network.hpp"

#include <algorithm>

#include "board_wifi.hpp"

namespace foresthub {
namespace platform {
namespace arduino {

ArduinoNetwork::ArduinoNetwork(const NetworkConfig& config)
    : ssid_(config.ssid ? config.ssid : ""), password_(config.password ? config.password : "") {}

std::string ArduinoNetwork::Connect(unsigned long timeout_ms) {
    if (WiFi.status() == WL_CONNECTED) {
        return "";
    }

    unsigned long start = millis();

#if defined(ARDUINO_ARCH_ESP32)
    // ESP32: begin() is async — call once, then poll status
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid_.c_str(), password_.c_str());

    unsigned long backoff = 100;
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start >= timeout_ms) {
            WiFi.disconnect();
            return "WiFi connection timeout after " + std::to_string(timeout_ms) + "ms";
        }
        yield();
        delay(backoff);
        backoff = std::min(backoff * 2, static_cast<unsigned long>(2000));
    }
#else
    // Portenta H7 / Mbed boards: begin() is blocking and returns connection status.
    // Must retry begin() itself — polling status() alone doesn't trigger reconnection.
    while (WiFi.begin(ssid_.c_str(), password_.c_str()) != WL_CONNECTED) {
        if (millis() - start >= timeout_ms) {
            return "WiFi connection timeout after " + std::to_string(timeout_ms) + "ms";
        }
        yield();
        delay(1000);
    }
#endif

    return "";
}

void ArduinoNetwork::Disconnect() {
    WiFi.disconnect();
}

NetworkStatus ArduinoNetwork::GetStatus() const {
    switch (WiFi.status()) {
        case WL_CONNECTED:
            return NetworkStatus::kConnected;
        case WL_IDLE_STATUS:
        case WL_DISCONNECTED:
            return NetworkStatus::kDisconnected;
        case WL_NO_SSID_AVAIL:
        case WL_CONNECT_FAILED:
        case WL_CONNECTION_LOST:
            return NetworkStatus::kFailed;
        default:
            return NetworkStatus::kDisconnected;
    }
}

std::string ArduinoNetwork::GetLocalIp() const {
    if (WiFi.status() == WL_CONNECTED) {
        return std::string(WiFi.localIP().toString().c_str());
    }
    return "0.0.0.0";
}

int ArduinoNetwork::GetSignalStrength() const {
    if (WiFi.status() == WL_CONNECTED) {
        return static_cast<int>(WiFi.RSSI());
    }
    return 0;
}

}  // namespace arduino
}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_ARDUINO && FORESTHUB_ENABLE_NETWORK
