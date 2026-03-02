#ifndef FORESTHUB_PLATFORM_PLATFORM_HPP
#define FORESTHUB_PLATFORM_PLATFORM_HPP

#include <cstdint>
#include <memory>

#include "foresthub/core/http_client.hpp"
#include "foresthub/platform/console.hpp"
#include "foresthub/platform/crypto.hpp"
#include "foresthub/platform/gpio.hpp"
#include "foresthub/platform/network.hpp"
#include "foresthub/platform/time.hpp"

namespace foresthub {
namespace platform {

/// Construction-time configuration passed to CreatePlatform().
///
/// Subsystem-organized: network credentials, console baud rate, etc.
/// Extend with additional subsystem configs as needed (e.g. CryptoConfig).
struct PlatformConfig {
    NetworkConfig network;             ///< Network credentials and options.
    unsigned long baud_rate = 115200;  ///< Console communication speed in bits per second.
};

/// Call-time configuration for HTTP client creation.
///
/// Passed to CreateHttpClient() to specify connection parameters.
/// Host is required; all other fields have sensible defaults.
struct HttpClientConfig {
    const char* host = nullptr;        ///< Target hostname (required).
    uint16_t port = 443;               ///< Target port number.
    bool use_tls = true;               ///< Whether to use TLS encryption.
    unsigned long timeout_ms = 30000;  ///< Request timeout in milliseconds.
};

/// Holds all platform subsystems and provides a factory for HTTP clients.
///
/// Created once at startup via CreatePlatform() and reused for the entire application lifetime.
class PlatformContext {
public:
    std::shared_ptr<NetworkInterface> network;  ///< Network connectivity subsystem.
    std::shared_ptr<ConsoleInterface> console;  ///< Console I/O subsystem.
    std::shared_ptr<TimeInterface> time;        ///< Time and delay subsystem.
    std::shared_ptr<CryptoInterface> crypto;    ///< TLS/crypto subsystem.
    std::shared_ptr<GpioInterface> gpio;  ///< GPIO pin I/O subsystem (nullptr if FORESTHUB_ENABLE_GPIO not defined).

    /// Create a platform-specific HTTP client.
    /// @param config Connection parameters (host, port, TLS, timeout).
    /// @return HTTP client configured for the given parameters.
    virtual std::shared_ptr<core::HttpClient> CreateHttpClient(const HttpClientConfig& config) = 0;

    virtual ~PlatformContext() = default;
};

/// Factory function. Returns the platform-specific context.
/// @param config Network and connectivity configuration.
/// @return Platform-specific context with all subsystems initialized.
std::shared_ptr<PlatformContext> CreatePlatform(const PlatformConfig& config = {});

}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_PLATFORM_HPP
