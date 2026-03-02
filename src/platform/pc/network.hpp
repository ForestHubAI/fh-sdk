#ifndef FORESTHUB_PLATFORM_PC_NETWORK_HPP
#define FORESTHUB_PLATFORM_PC_NETWORK_HPP

#include "foresthub/platform/network.hpp"

namespace foresthub {
namespace platform {
namespace pc {

/// Mock network for PC. The OS handles networking; connect/disconnect are no-ops.
class PcNetwork : public NetworkInterface {
public:
    /// No-op; OS networking is always available.
    std::string Connect(unsigned long timeout_ms = 10000) override;

    /// No-op; OS networking cannot be disconnected.
    void Disconnect() override;
    /// Always returns kConnected.
    NetworkStatus GetStatus() const override;
    /// Always returns "127.0.0.1".
    std::string GetLocalIp() const override;
    /// Always returns 0 (not applicable on PC).
    int GetSignalStrength() const override;
};

}  // namespace pc
}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_PC_NETWORK_HPP
