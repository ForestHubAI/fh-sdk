#ifdef FORESTHUB_PLATFORM_PC  // Translation Unit Guard: Only compile on PC

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

#endif  // FORESTHUB_PLATFORM_PC
