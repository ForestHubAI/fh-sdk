// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PLATFORM_TIME_HPP
#define FORESTHUB_PLATFORM_TIME_HPP

/// @file
/// Abstract interface for time operations and synchronization.

#include <ctime>
#include <string>

namespace foresthub {
namespace hal {

/// Call-time configuration for time synchronization.
///
/// Passed to SyncTime() to control the synchronization source and behavior.
/// All fields have sensible defaults, so `SyncTime()` works without arguments.
struct TimeConfig {
    const char* time_server = "pool.ntp.org";  ///< Hostname of the time server.
    long std_offset_sec = 0;                   ///< Timezone standard offset from UTC in seconds (e.g., 3600 for CET).
    int dst_offset_sec = 0;           ///< Additional daylight saving offset in seconds (e.g., 3600 when active).
    unsigned long timeout_ms = 5000;  ///< Maximum wait for synchronization in milliseconds.
};

/// Abstract interface for time operations.
class Time {
public:
    virtual ~Time() = default;

    /// Synchronize system time with an external source.
    /// @param config Time synchronization parameters (server, offsets, timeout).
    /// @return Error message on failure, empty on success.
    virtual std::string SyncTime(const TimeConfig& config = {}) = 0;

    /// Get current epoch time in seconds.
    /// @return Seconds since 1970-01-01 00:00:00 UTC (Unix epoch).
    virtual unsigned long GetEpochTime() const = 0;

    /// Get milliseconds since startup (or arbitrary reference point).
    /// @return Milliseconds elapsed; wraps after ~49.7 days on 32-bit platforms.
    virtual unsigned long GetMillis() const = 0;

    /// Delay execution for the specified number of milliseconds.
    /// @param ms Duration in milliseconds.
    virtual void Delay(unsigned long ms) const = 0;

    /// Check if time has been synchronized.
    /// @param min_epoch Minimum epoch (seconds) to consider synced (default: 2026-01-01 00:00:00 UTC).
    /// @return True if current epoch time is at or above min_epoch.
    virtual bool IsSynced(unsigned long min_epoch = 1767225600) const = 0;

    /// Set timezone offset locally (no network I/O). Overrides offset from SyncTime().
    /// @param std_offset_sec Timezone standard offset from UTC in seconds (e.g., 3600 for CET).
    /// @param dst_offset_sec Additional daylight saving offset in seconds (e.g., 3600 when active).
    virtual void SetOffset(long std_offset_sec, int dst_offset_sec) = 0;

    /// Get total UTC offset (standard + DST) in seconds.
    virtual long utc_offset_sec() const = 0;

    /// Get current local epoch time (UTC + timezone offset) in seconds.
    ///
    /// Convenience method combining GetEpochTime() and utc_offset_sec().
    /// @return Local epoch seconds. Safe for all real-world offsets (UTC-12 to UTC+14).
    unsigned long GetLocalEpoch() const {
        return static_cast<unsigned long>(static_cast<long>(GetEpochTime()) + utc_offset_sec());
    }

    /// Get current local time as calendar fields (UTC + stored timezone offset).
    /// @param result Output struct filled with local time fields.
    virtual void GetLocalTime(struct tm& result) const = 0;
};

}  // namespace hal
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_TIME_HPP
