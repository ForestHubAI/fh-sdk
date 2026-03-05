#ifndef FORESTHUB_PLATFORM_ARDUINO_BOARD_CORE_HPP
#define FORESTHUB_PLATFORM_ARDUINO_BOARD_CORE_HPP

// =============================================================================
// Board-Specific Base Types (WiFi-Free)
// =============================================================================
//
// Provides base Arduino types needed by non-WiFi code.
//
// Supported boards:
//   - ESP32 (all variants: DevKit, S2, S3, C3, etc.)
//   - Arduino Portenta H7 (M7 and M4 cores)
// =============================================================================

#if defined(ARDUINO_ARCH_ESP32)
// -------------------------------------------------------------------------
// ESP32: Arduino.h provides base types (Stream, Client) in global namespace.
// -------------------------------------------------------------------------
#include <Arduino.h>

#elif defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_PORTENTA_H7_M4)
// -------------------------------------------------------------------------
// Portenta H7: Arduino Mbed framework puts types in arduino:: namespace.
// Pull base types into global namespace for ESP32 API compatibility.
// -------------------------------------------------------------------------
#include <Arduino.h>

using arduino::Stream;

#else
#error "Unsupported Arduino board. Supported: ESP32, Portenta H7"

#endif

#endif  // FORESTHUB_PLATFORM_ARDUINO_BOARD_CORE_HPP
