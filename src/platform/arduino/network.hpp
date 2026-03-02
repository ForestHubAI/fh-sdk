#ifndef FORESTHUB_PLATFORM_ARDUINO_NETWORK_HPP
#define FORESTHUB_PLATFORM_ARDUINO_NETWORK_HPP

#ifdef FORESTHUB_ENABLE_NETWORK

#include <string>

#include "foresthub/platform/network.hpp"

namespace foresthub {
namespace platform {
namespace arduino {

/// WiFi network implementation for Arduino using station mode.
class ArduinoNetwork : public NetworkInterface {
public:
    /// Construct with network configuration for station mode.
    explicit ArduinoNetwork(const NetworkConfig& config);

    /// Connects via WiFi.begin() with timeout and retry.
    std::string Connect(unsigned long timeout_ms = 30000) override;

    /// Calls WiFi.disconnect().
    void Disconnect() override;
    /// Maps WiFi.status() to NetworkStatus enum.
    NetworkStatus GetStatus() const override;
    /// Returns WiFi.localIP() as string.
    std::string GetLocalIp() const override;
    /// Returns WiFi.RSSI() signal strength in dBm.
    int GetSignalStrength() const override;

private:
    std::string ssid_;      ///< Network SSID (deep copy from NetworkConfig).
    std::string password_;  ///< Network password (deep copy from NetworkConfig).
};

}  // namespace arduino
}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_ENABLE_NETWORK

#endif  // FORESTHUB_PLATFORM_ARDUINO_NETWORK_HPP
