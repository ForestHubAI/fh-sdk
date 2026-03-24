// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

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
