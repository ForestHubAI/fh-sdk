#ifndef FORESTHUB_PLATFORM_ARDUINO_BOARD_DEFS_HPP
#define FORESTHUB_PLATFORM_ARDUINO_BOARD_DEFS_HPP

// =============================================================================
// Board-Specific Library Selection
// =============================================================================
//
// Each Arduino board family uses different WiFi and TLS libraries.
// This header provides unified includes and type aliases.
//
// Supported boards:
//   - ESP32 (all variants: DevKit, S2, S3, C3, etc.)
//   - Arduino Portenta H7 (M7 and M4 cores)
//
// Usage:
//   #include "board_defs.hpp"
//   // WiFi, WiFiClient, and SecureClient are now available
//
// =============================================================================

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
// Arduino Mbed framework puts types in arduino:: namespace.
// Pull them into global namespace for ESP32 API compatibility.
// -------------------------------------------------------------------------
#include <WiFi.h>
#include <WiFiSSLClient.h>

using arduino::Client;
using arduino::Stream;
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
#error "Unsupported Arduino board. Supported: ESP32, Portenta H7"

#endif

#endif  // FORESTHUB_PLATFORM_ARDUINO_BOARD_DEFS_HPP
