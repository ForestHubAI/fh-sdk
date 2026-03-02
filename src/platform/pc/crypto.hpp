#ifndef FORESTHUB_PLATFORM_PC_CRYPTO_HPP
#define FORESTHUB_PLATFORM_PC_CRYPTO_HPP

#include "foresthub/platform/crypto.hpp"

namespace foresthub {
namespace platform {
namespace pc {

/// Crypto implementation for PC. TLS is handled by CPR/libcurl using system certs.
class PcCrypto : public CryptoInterface {
public:
    /// Always returns nullptr; TLS is handled by CPR/libcurl.
    std::shared_ptr<TLSClientWrapper> CreateTlsClient(const char* root_ca = nullptr,
                                                      unsigned long timeout_ms = 5000) override;

    /// Always returns nullptr; system certs are used by CPR/libcurl.
    const char* GetGtsRootCerts() const override;
    /// Always returns true; system TLS is always available.
    bool IsAvailable() const override;
};

}  // namespace pc
}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_PC_CRYPTO_HPP
