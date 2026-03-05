#ifndef FORESTHUB_PLATFORM_ARDUINO_BOARD_WIFI_HPP
#define FORESTHUB_PLATFORM_ARDUINO_BOARD_WIFI_HPP

// =============================================================================
// Board-Specific WiFi/TLS Headers and Type Aliases
// =============================================================================
//
// Provides WiFi/TLS includes and the SecureClient type alias.
// Depends on board_core.hpp for base types.
//
// Supported boards:
//   - ESP32 (all variants: DevKit, S2, S3, C3, etc.)
//   - Arduino Portenta H7 (M7 and M4 cores)
// =============================================================================

#include "board_core.hpp"

#if defined(ARDUINO_ARCH_ESP32)
// -------------------------------------------------------------------------
// ESP32: WiFi.h + WiFiClientSecure
// -------------------------------------------------------------------------
#include <WiFi.h>
#include <WiFiClientSecure.h>

namespace foresthub {
namespace platform {
namespace arduino {
/// Board-specific TLS-capable client type alias.
using SecureClient = WiFiClientSecure;
}  // namespace arduino
}  // namespace platform
}  // namespace foresthub

#elif defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_PORTENTA_H7_M4)
// -------------------------------------------------------------------------
// Portenta H7: WiFi.h (Mbed) + WiFiSSLClient
// Pull WiFiClient into global namespace for ESP32 API compatibility.
// -------------------------------------------------------------------------
#include <WiFi.h>
#include <WiFiSSLClient.h>

using arduino::Client;
using arduino::WiFiClient;

namespace foresthub {
namespace platform {
namespace arduino {
/// Board-specific TLS-capable client type alias.
using SecureClient = WiFiSSLClient;
}  // namespace arduino
}  // namespace platform
}  // namespace foresthub

#else
#error "Unsupported Arduino board for WiFi/TLS. Supported: ESP32, Portenta H7"

#endif

#endif  // FORESTHUB_PLATFORM_ARDUINO_BOARD_WIFI_HPP
