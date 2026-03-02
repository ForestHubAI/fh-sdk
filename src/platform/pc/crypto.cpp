#ifdef FORESTHUB_PLATFORM_PC  // Translation Unit Guard: Only compile on PC

#include "crypto.hpp"

namespace foresthub {
namespace platform {
namespace pc {

std::shared_ptr<TLSClientWrapper> PcCrypto::CreateTlsClient(const char* /*root_ca*/, unsigned long /*timeout_ms*/) {
    // On PC, TLS is managed by CPR/libcurl using the system certificate store.
    // No separate TLS client wrapper is needed.
    return nullptr;
}

const char* PcCrypto::GetGtsRootCerts() const {
    // PC uses the system certificate store. No embedded certs needed.
    return nullptr;
}

bool PcCrypto::IsAvailable() const {
    return true;  // CPR/libcurl always provides TLS on PC.
}

}  // namespace pc
}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_PC
