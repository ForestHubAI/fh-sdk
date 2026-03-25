// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PLATFORM_ARDUINO_CRYPTO_HPP
#define FORESTHUB_PLATFORM_ARDUINO_CRYPTO_HPP

#ifdef FORESTHUB_ENABLE_CRYPTO

#include <memory>

#include "board_wifi.hpp"
#include "foresthub/hal/crypto.hpp"

namespace foresthub {
namespace hal {
namespace arduino {

/// TLSClientWrapper implementation wrapping the board-specific TLS client with root CA certificates.
class ArduinoTLSClient : public TLSClientWrapper {
public:
    /// Construct a TLS client with the given root CA and timeout.
    ArduinoTLSClient(const char* root_ca, unsigned long timeout_ms);

    ~ArduinoTLSClient() override;

    /// Returns a pointer to the underlying TLS client.
    void* GetNativeClient() override;

private:
    std::unique_ptr<SecureClient> client_;
};

/// Crypto implementation providing TLS client creation with bundled GTS root certificates.
class ArduinoCrypto : public Crypto {
public:
    ArduinoCrypto() = default;

    /// Falls back to bundled GTS root certs when root_ca is nullptr.
    std::shared_ptr<TLSClientWrapper> CreateTlsClient(const char* root_ca = nullptr,
                                                      unsigned long timeout_ms = 60000) override;

    /// Returns the bundled GTS root CA PEM string.
    const char* GetGtsRootCerts() const override;

    /// Always returns true — TLS capability exists if code compiled.
    bool IsAvailable() const override;
};

}  // namespace arduino
}  // namespace hal
}  // namespace foresthub

#endif  // FORESTHUB_ENABLE_CRYPTO

#endif  // FORESTHUB_PLATFORM_ARDUINO_CRYPTO_HPP
