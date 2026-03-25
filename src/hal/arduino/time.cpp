// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifdef FORESTHUB_PLATFORM_ARDUINO  // Translation Unit Guard: Only compile on Arduino

#include "time.hpp"

#include <Arduino.h>

#include <cstring>
#include <ctime>
#include <string>

#include "board_wifi.hpp"

#if defined(ARDUINO_PORTENTA_H7_M7)
#include <WiFiUdp.h>
#endif

namespace foresthub {
namespace hal {
namespace arduino {

// =============================================================================
// Configuration Constants
// =============================================================================

// Maximum number of NTP sync attempts before returning an error.
static constexpr int kMaxSyncAttempts = 3;

// Delay between retry attempts in milliseconds.
static constexpr unsigned long kRetryDelayMs = 1000;

// Polling interval while waiting for NTP response within a single attempt.
static constexpr unsigned long kPollIntervalMs = 200;

// Step size for yield()-protected delay in milliseconds.
static constexpr unsigned long kDelayStepMs = 10;

// Minimum epoch for time validity check (2026-01-01 00:00:00 UTC).
static constexpr unsigned long kMinValidEpoch = 1767225600;

// =============================================================================
// Portenta H7 M7: Manual NTP via raw UDP + set_time()
// =============================================================================
//
// The Portenta H7 (Mbed OS) has no configTime(). We send a 48-byte NTP request
// via WiFiUDP, parse the response, and call set_time() to initialize the RTC.
// After set_time(), time(nullptr) returns the correct UTC epoch.

#if defined(ARDUINO_PORTENTA_H7_M7)

// set_time() initializes the STM32H7 hardware RTC. Declared in mbed_rtc_time.h,
// but we forward-declare it to avoid pulling in the massive <mbed.h> header.
// After this call, time(nullptr) returns the correct UTC epoch.
extern "C" void set_time(time_t t);

// NTP protocol packet size (fixed by RFC 5905).
static constexpr int kNtpPacketSize = 48;

// Seconds between NTP epoch (1900-01-01) and Unix epoch (1970-01-01).
static constexpr unsigned long kNtpUnixOffset = 2208988800UL;

// Standard NTP server port.
static constexpr unsigned int kNtpPort = 123;

// Arbitrary local port for outgoing UDP socket (non-privileged range).
static constexpr unsigned int kNtpLocalPort = 2390;

// How often to check for the NTP response. 50ms is short enough for
// responsive behavior, long enough to avoid busy-spinning.
static constexpr unsigned long kNtpResponsePollMs = 50;

// Construct and send a minimal NTP client request (RFC 5905).
//
// Only the first byte matters for a client request. The server ignores all
// other fields and responds with its current timestamp. We zero-fill the
// remaining 47 bytes as required by the protocol.
static bool SendNtpPacket(WiFiUDP& udp, const char* server) {
    uint8_t packet[kNtpPacketSize];
    memset(packet, 0, kNtpPacketSize);

    // Byte 0 encodes three fields: LI (2 bits), Version (3 bits), Mode (3 bits).
    // 0xE3 = 11 100 011 = LI=3 (unsynchronized), Version=4 (current), Mode=3 (client)
    packet[0] = 0xE3;

    // beginPacket resolves the hostname via DNS and opens the UDP connection.
    // Returns false if DNS fails or the network is unreachable.
    if (!udp.beginPacket(server, kNtpPort)) {
        return false;
    }
    udp.write(packet, kNtpPacketSize);
    return (udp.endPacket() != 0);
}

// Perform a single NTP exchange: send request, wait for response, parse epoch.
//
// The NTP response contains a "Transmit Timestamp" at bytes 40-43 representing
// seconds since 1900-01-01. We subtract kNtpUnixOffset to convert to Unix epoch
// (seconds since 1970-01-01), which is what set_time() and time(nullptr) expect.
// Returns Unix epoch (UTC) on success, 0 on timeout or invalid response.
static unsigned long NtpQueryEpoch(const char* server, unsigned long timeout_ms) {
    WiFiUDP udp;
    udp.begin(kNtpLocalPort);

    if (!SendNtpPacket(udp, server)) {
        udp.stop();
        return 0;
    }

    // The NTP response takes ~20-50ms over the internet. Poll parsePacket()
    // until data arrives. yield() prevents watchdog resets on RTOS boards.
    unsigned long start = millis();
    while (udp.parsePacket() == 0) {
        if ((millis() - start) >= timeout_ms) {
            udp.stop();
            return 0;
        }
        ::delay(kNtpResponsePollMs);
        yield();
    }

    uint8_t packet[kNtpPacketSize];
    udp.read(packet, kNtpPacketSize);
    udp.stop();

    // Extract Transmit Timestamp from bytes 40-43 (big-endian, 32-bit unsigned).
    // This is the server's time at the moment it sent the response.
    unsigned long high_word = (static_cast<unsigned long>(packet[40]) << 8) | packet[41];
    unsigned long low_word = (static_cast<unsigned long>(packet[42]) << 8) | packet[43];
    unsigned long secs_since_1900 = (high_word << 16) | low_word;

    if (secs_since_1900 == 0) {
        return 0;  // Server sent an empty timestamp -- treat as invalid
    }

    // Convert from NTP epoch (1900) to Unix epoch (1970)
    return secs_since_1900 - kNtpUnixOffset;
}

#endif  // ARDUINO_PORTENTA_H7_M7

// =============================================================================
// NTP Synchronization
// =============================================================================

std::string ArduinoTime::SyncTime(const TimeConfig& config) {
    // Store timezone offsets for GetLocalTime() / utc_offset_sec()
    // regardless of whether NTP sync is needed.
    std_offset_sec_ = config.std_offset_sec;
    dst_offset_sec_ = config.dst_offset_sec;

    // Already synchronized -- skip NTP traffic
    if (GetEpochTime() >= kMinValidEpoch) {
        return "";
    }

    // WiFi must be connected for NTP to work
    if (WiFi.status() != WL_CONNECTED) {
        return "NTP sync failed: WiFi not connected";
    }

    // Configure NTP (idempotent -- safe to call multiple times)
#if defined(ARDUINO_ARCH_ESP32)
    last_server_ = config.time_server;
    configTime(config.std_offset_sec, config.dst_offset_sec, config.time_server);
#elif defined(ARDUINO_PORTENTA_H7_M7)
    // Portenta H7 M7: Manual NTP via raw UDP + set_time().
    // gmtOffset/dstOffset are unused -- set_time() stores UTC, and time(nullptr)
    // always returns UTC per the C standard. Offsets only affect localtime(),
    // which we don't call. Adding offsets here would corrupt the RTC and break
    // TLS certificate validation (which compares against UTC).

    // Retry NTP up to kMaxSyncAttempts times (same resilience as ESP32 path).
    for (int attempt = 1; attempt <= kMaxSyncAttempts; ++attempt) {
        unsigned long epoch = NtpQueryEpoch(config.time_server, config.timeout_ms);
        if (epoch >= kMinValidEpoch) {
            set_time(static_cast<time_t>(epoch));
            break;  // Fall through to validation loop below
        }
        if (attempt < kMaxSyncAttempts) {
            Delay(kRetryDelayMs);
        }
    }
    // If set_time() succeeded, the validation loop below passes instantly.
    // If all attempts failed, time(nullptr) is still 0, and the validation
    // loop will timeout and return the error message.

#else
    // Unknown board -- no NTP mechanism available.
    (void)config;
    return "NTP sync not implemented for this board";
#endif

    // Retry loop: attempt NTP sync up to kMaxSyncAttempts times
    for (int attempt = 1; attempt <= kMaxSyncAttempts; ++attempt) {
        const unsigned long start = millis();
        time_t now = time(nullptr);

        // Poll until valid time or timeout
        while (now < static_cast<time_t>(kMinValidEpoch) && (millis() - start) < config.timeout_ms) {
            Delay(kPollIntervalMs);
            now = time(nullptr);
        }

        if (now >= static_cast<time_t>(kMinValidEpoch)) {
            return "";  // Success
        }

        // Pause before next attempt (skip after last attempt)
        if (attempt < kMaxSyncAttempts) {
            Delay(kRetryDelayMs);
        }
    }

    return "NTP sync failed after " + std::to_string(kMaxSyncAttempts) +
           " attempts (timeout: " + std::to_string(config.timeout_ms) + "ms)";
}

// =============================================================================
// Time Queries
// =============================================================================

unsigned long ArduinoTime::GetEpochTime() const {
    return static_cast<unsigned long>(time(nullptr));
}

unsigned long ArduinoTime::GetMillis() const {
    return millis();
}

// =============================================================================
// Delay (WDT-safe)
// =============================================================================

void ArduinoTime::Delay(unsigned long ms) const {
    // Split into small steps with yield() to prevent ESP32 WDT resets.
    while (ms > kDelayStepMs) {
        ::delay(kDelayStepMs);
        yield();
        ms -= kDelayStepMs;
    }

    if (ms > 0) {
        ::delay(ms);
        yield();
    }
}

// =============================================================================
// Sync Status
// =============================================================================

bool ArduinoTime::IsSynced(unsigned long min_epoch) const {
    return GetEpochTime() >= min_epoch;
}

// =============================================================================
// Timezone Support
// =============================================================================

void ArduinoTime::SetOffset(long std_offset_sec, int dst_offset_sec) {
    std_offset_sec_ = std_offset_sec;
    dst_offset_sec_ = dst_offset_sec;

#if defined(ARDUINO_ARCH_ESP32)
    configTime(std_offset_sec, dst_offset_sec, last_server_);
#endif
}

long ArduinoTime::utc_offset_sec() const {
    return std_offset_sec_ + dst_offset_sec_;
}

void ArduinoTime::GetLocalTime(struct tm& result) const {
    time_t local = static_cast<time_t>(GetEpochTime()) + utc_offset_sec();
    gmtime_r(&local, &result);
    result.tm_isdst = (dst_offset_sec_ != 0) ? 1 : 0;
}

}  // namespace arduino
}  // namespace hal
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_ARDUINO
