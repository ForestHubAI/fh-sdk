// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PLATFORM_CRYPTO_HPP
#define FORESTHUB_PLATFORM_CRYPTO_HPP

/// @file
/// Abstract interfaces for TLS/crypto operations.

#include <memory>

namespace foresthub {
namespace platform {

/// Wrapper around a platform-specific TLS client.
class TLSClientWrapper {
public:
    virtual ~TLSClientWrapper() = default;

    /// Access the underlying platform-specific client.
    /// @return Pointer to the native TLS client; type is platform-specific.
    ///         The wrapper retains ownership; the pointer is valid only while the wrapper is alive.
    virtual void* GetNativeClient() = 0;
};

/// Abstract interface for TLS/crypto operations.
class Crypto {
public:
    virtual ~Crypto() = default;

    /// Create a TLS-enabled client with optional root CA certificate.
    /// @param root_ca PEM-encoded root CA certificate, or nullptr for platform default.
    /// @param timeout_ms Connection timeout in milliseconds.
    /// @return Shared pointer to the TLS client wrapper.
    virtual std::shared_ptr<TLSClientWrapper> CreateTlsClient(const char* root_ca = nullptr,
                                                              unsigned long timeout_ms = 5000) = 0;

    /// Get the bundled Google Trust Services root certificates (for Google Cloud endpoints).
    /// @return PEM-encoded certificate string with static lifetime.
    virtual const char* GetGtsRootCerts() const = 0;

    /// Check if TLS/crypto subsystem is available.
    virtual bool IsAvailable() const = 0;
};

}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_CRYPTO_HPP
