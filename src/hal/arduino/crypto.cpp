// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#if defined(FORESTHUB_PLATFORM_ARDUINO) && defined(FORESTHUB_ENABLE_CRYPTO)

#include "crypto.hpp"

#include <cstring>

#include "board_wifi.hpp"
#include "hal/common/tls_certificates.hpp"

namespace foresthub {
namespace hal {
namespace arduino {

// ----------------------------------------------------------------------------
// ArduinoTLSClient
// ----------------------------------------------------------------------------

ArduinoTLSClient::ArduinoTLSClient(const char* root_ca, unsigned long timeout_ms) {
    client_ = std::make_unique<SecureClient>();

#if defined(ARDUINO_ARCH_ESP32)
    // ESP32: setCACert() accepts PEM directly
    client_->setCACert(root_ca);
    client_->setTimeout(timeout_ms / 1000);

#elif defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_PORTENTA_H7_M4)
    // Portenta H7: appendCustomCACert() + Stream::setTimeout (takes ms, not seconds)
    client_->appendCustomCACert(root_ca);
    client_->setTimeout(timeout_ms);
#endif
}

ArduinoTLSClient::~ArduinoTLSClient() = default;

void* ArduinoTLSClient::GetNativeClient() {
    return client_.get();
}

// ----------------------------------------------------------------------------
// ArduinoCrypto
// ----------------------------------------------------------------------------

std::shared_ptr<TLSClientWrapper> ArduinoCrypto::CreateTlsClient(const char* root_ca, unsigned long timeout_ms) {
    // Use provided CA or default to GTS roots
    const char* effective_ca = (root_ca && *root_ca) ? root_ca : foresthub::platform::common::kGtsRootCertificatesPem;

    return std::make_shared<ArduinoTLSClient>(effective_ca, timeout_ms);
}

const char* ArduinoCrypto::GetGtsRootCerts() const {
    return foresthub::platform::common::kGtsRootCertificatesPem;
}

bool ArduinoCrypto::IsAvailable() const {
    return true;
}

}  // namespace arduino
}  // namespace hal
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_ARDUINO && FORESTHUB_ENABLE_CRYPTO
