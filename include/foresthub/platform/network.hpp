// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PLATFORM_NETWORK_HPP
#define FORESTHUB_PLATFORM_NETWORK_HPP

/// @file
/// Abstract interface for network connectivity.

#include <cstdint>
#include <string>

namespace foresthub {
namespace platform {

/// Connection state of the network subsystem.
enum class NetworkStatus : uint8_t {
    kDisconnected,  ///< Not connected (clean state or after Disconnect()).
    kConnecting,    ///< Connection attempt in progress.
    kConnected,     ///< Successfully connected and ready.
    kFailed         ///< Connection attempt failed.
};

/// Construction-time configuration for the network subsystem.
///
/// Transport-specific credentials and options passed to the concrete implementation
/// at construction time. Extend with additional fields (e.g. APN, static IP)
/// for other network transports.
struct NetworkConfig {
    const char* ssid = nullptr;      ///< Network identifier (nullptr to skip setup).
    const char* password = nullptr;  ///< Network password (nullptr for open networks).
};

/// Abstract interface for network connectivity.
///
/// Transport-specific configuration (credentials, etc.) belongs in the concrete
/// class constructor via NetworkConfig. This interface only handles
/// activation/deactivation and status.
class NetworkInterface {
public:
    virtual ~NetworkInterface() = default;

    /// Activate the network connection.
    /// Transport-specific parameters (SSID, APN, etc.) are set at construction time.
    /// @param timeout_ms Maximum time to wait for connection in milliseconds.
    /// @return Error message on failure, empty on success.
    virtual std::string Connect(unsigned long timeout_ms = 30000) = 0;

    /// Disconnect from the network.
    virtual void Disconnect() = 0;

    /// Get the current connection status.
    virtual NetworkStatus GetStatus() const = 0;

    /// Get the local IP address as string.
    /// @return Dotted-decimal IP address, or empty string on error.
    virtual std::string GetLocalIp() const = 0;

    /// Get signal strength in dBm.
    /// @return RSSI in dBm, or 0 if not applicable.
    virtual int GetSignalStrength() const = 0;
};

}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_NETWORK_HPP
