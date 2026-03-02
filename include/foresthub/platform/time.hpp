#ifndef FORESTHUB_PLATFORM_TIME_HPP
#define FORESTHUB_PLATFORM_TIME_HPP

#include <ctime>
#include <string>

namespace foresthub {
namespace platform {

/// Call-time configuration for time synchronization.
///
/// Passed to SyncTime() to control the synchronization source and behavior.
/// All fields have sensible defaults, so `SyncTime()` works without arguments.
struct TimeConfig {
    const char* time_server = "pool.ntp.org";  ///< Hostname of the time server.
    long gmt_offset_sec = 0;                   ///< Offset from UTC in seconds.
    int daylight_offset_sec = 0;               ///< Daylight saving offset in seconds.
    unsigned long timeout_ms = 5000;           ///< Maximum wait for synchronization in milliseconds.
};

/// Abstract interface for time operations.
class TimeInterface {
public:
    virtual ~TimeInterface() = default;

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
    /// @param gmt_offset_sec Offset from UTC in seconds (e.g., 3600 for CET).
    /// @param daylight_offset_sec Additional daylight saving offset in seconds.
    virtual void SetOffset(long gmt_offset_sec, int daylight_offset_sec) = 0;

    /// Get total timezone offset (GMT + daylight saving) in seconds.
    virtual long gmt_offset_sec() const = 0;

    /// Get current local time as calendar fields (UTC + stored timezone offset).
    /// @param result Output struct filled with local time fields.
    virtual void GetLocalTime(struct tm& result) const = 0;
};

}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_TIME_HPP
