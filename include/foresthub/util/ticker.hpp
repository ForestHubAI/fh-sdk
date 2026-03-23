// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_UTIL_TICKER_HPP
#define FORESTHUB_UTIL_TICKER_HPP

/// @file
/// Periodic, one-shot, and calendar-aligned scheduling utility.

namespace foresthub {
namespace util {

/// Unified timing class for periodic, one-shot, and calendar-aligned scheduling.
///
/// Platform-independent — takes any `unsigned long` time source. Two evaluation paths:
/// - **Relative** (Periodic/OneShot): Uses unsigned subtraction (`time - start_time_`).
///   Wrap-safe for 32-bit unsigned overflow. Drift correction maintains cadence after gaps.
/// - **Absolute** (Daily/Weekly/Hourly): Uses signed slot math (`FloorDiv(time + offset, period)`).
///   Calendar-aligned via epoch seconds. No `<ctime>` dependency.
///
/// Factory methods create pre-configured instances for common patterns.
/// The caller provides the current time to `Check(time)` — the Ticker decides internally
/// whether to use relative or absolute evaluation.
///
/// @code
/// // Periodic: read sensor every 5 seconds (now_ms = any millisecond source)
/// auto ticker = Ticker::Periodic(5000);
/// ticker.Start(now_ms);
/// if (ticker.Check(now_ms)) { readSensor(); }
///
/// // Daily: sync data at 14:00 Berlin time (now_epoch = UTC epoch seconds)
/// auto daily = Ticker::Daily(14, 0).WithTimezoneOffset(3600);
/// daily.Start(now_epoch);
/// if (daily.Check(now_epoch)) { syncData(); }
///
/// // OneShot: timeout after 30 seconds
/// auto timeout = Ticker::OneShot(30000);
/// timeout.Start(now_ms);
/// if (timeout.Check(now_ms)) { handleTimeout(); }
/// @endcode
class Ticker {
public:
    /// Construct a Ticker with custom period and offset.
    ///
    /// For most use cases, prefer the static factory methods (Daily, Weekly, Hourly, Periodic, OneShot).
    /// @param period Period in time units (seconds for calendar-based, milliseconds for relative).
    /// @param offset Internal offset for slot calculation (calendar-based scheduling only).
    explicit Ticker(unsigned long period, long offset = 0)
        : period_(period),
          offset_(offset),
          tz_offset_sec_(0),
          relative_(false),
          start_time_(0),
          last_slot_(0),
          fire_count_(0),
          missed_ticks_(0),
          max_fires_(0),
          running_(false) {}

    // -- Factory methods ------------------------------------------------------

    /// Create a periodic ticker that fires at a fixed interval.
    ///
    /// Uses relative mode (wrap-safe unsigned subtraction). Time unit is determined
    /// by the caller (e.g., milliseconds or ticks).
    /// @param interval Interval between fires (e.g., 5000 for 5 seconds at millisecond resolution).
    static Ticker Periodic(unsigned long interval) {
        Ticker t(interval);
        t.relative_ = true;
        return t;
    }

    /// Create a one-shot timer that fires exactly once after a delay.
    ///
    /// Equivalent to `Periodic(delay).WithMaxFires(1)`.
    /// @param delay Delay before firing (same unit as Periodic interval).
    static Ticker OneShot(unsigned long delay) {
        Ticker t = Periodic(delay);
        t.max_fires_ = 1;
        return t;
    }

    /// Create a daily ticker that fires once per day at a specific hour and minute.
    ///
    /// Uses absolute mode with epoch seconds. For local time scheduling,
    /// use `WithTimezoneOffset(utc_offset_sec)` and pass raw UTC epoch to Check().
    /// @param hour Hour of day (0-23).
    /// @param minute Minute of hour (0-59).
    static Ticker Daily(int hour, int minute = 0) {
        return Ticker(86400UL, kEpochSundayOffset - (static_cast<long>(hour) * 3600L + minute * 60L));
    }

    /// Create a weekly ticker that fires once per week on a specific day and time.
    ///
    /// @param weekday Day of week (0=Sunday, 1=Monday, ..., 6=Saturday).
    /// @param hour Hour of day (0-23).
    /// @param minute Minute of hour (0-59).
    static Ticker Weekly(int weekday, int hour, int minute = 0) {
        return Ticker(604800UL, kEpochSundayOffset - (static_cast<long>(weekday) * 86400L +
                                                      static_cast<long>(hour) * 3600L + minute * 60L));
    }

    /// Create an hourly ticker that fires once per hour at a specific minute.
    ///
    /// @param minute Minute of hour (0-59).
    static Ticker Hourly(int minute = 0) { return Ticker(3600UL, -(minute * 60L)); }

    // -- Builder --------------------------------------------------------------

    /// Set the maximum number of fires before auto-stopping (0 = unlimited).
    Ticker& WithMaxFires(unsigned long n) {
        max_fires_ = n;
        return *this;
    }

    /// Set timezone offset for calendar-based scheduling (Daily/Weekly/Hourly).
    ///
    /// Applied internally in slot calculation — pass raw UTC epoch to Start()/Check().
    /// Mutually exclusive with manual offset addition at the call site.
    /// For DST changes at runtime, use SetTimezoneOffset().
    /// @param tz_offset_sec Total timezone offset from UTC in seconds (e.g., 7200 for CEST).
    Ticker& WithTimezoneOffset(long tz_offset_sec) {
        tz_offset_sec_ = tz_offset_sec;
        return *this;
    }

    // -- Lifecycle ------------------------------------------------------------

    /// Start (or restart) the ticker with the given time reference.
    ///
    /// Resets fire_count and missed ticks. For relative mode, pass a millisecond
    /// or tick counter. For calendar-based mode, pass UTC epoch seconds.
    /// @param time Current time value.
    void Start(unsigned long time) {
        if (relative_) {
            start_time_ = time;
        } else {
            last_slot_ = Slot(time);
        }
        fire_count_ = 0;
        missed_ticks_ = 0;
        running_ = true;
    }

    /// Check if the ticker should fire at the given time.
    ///
    /// Returns true at most once per period. After a long gap, fires once and
    /// reports the number of skipped intervals via missed_ticks().
    ///
    /// @param time Current time value (same unit as Start()).
    bool Check(unsigned long time) {
        if (!running_) return false;

        if (relative_) {
            // Wrap-safe unsigned path (immune to 32-bit unsigned overflow).
            unsigned long elapsed = time - start_time_;
            if (elapsed < period_) return false;
            missed_ticks_ = (elapsed / period_) - 1;
            start_time_ += (missed_ticks_ + 1) * period_;  // Drift correction: snap to grid.
        } else {
            // Slot-based path for calendar/epoch scheduling.
            long current_slot = Slot(time);
            if (current_slot <= last_slot_) return false;
            missed_ticks_ = static_cast<unsigned long>(current_slot - last_slot_ - 1);
            last_slot_ = current_slot;
        }

        ++fire_count_;
        if (max_fires_ > 0 && fire_count_ >= max_fires_) running_ = false;
        return true;
    }

    /// Stop the ticker. Call Start() to restart.
    void Stop() { running_ = false; }

    /// Update timezone offset at runtime (e.g., after DST transition).
    /// @param tz_offset_sec Total timezone offset from UTC in seconds.
    void SetTimezoneOffset(long tz_offset_sec) { tz_offset_sec_ = tz_offset_sec; }

    // -- Getters --------------------------------------------------------------

    /// Whether the ticker is currently running.
    bool running() const { return running_; }

    /// Number of times the ticker has fired since last Start().
    unsigned long fire_count() const { return fire_count_; }

    /// Number of intervals skipped in the last Check() that fired.
    ///
    /// Returns 0 when Check() fires on time. Only meaningful after a Check() that returned true.
    unsigned long missed_ticks() const { return missed_ticks_; }

    /// Configured period in time units.
    unsigned long period() const { return period_; }

    /// Configured timezone offset in seconds (0 if not set).
    long tz_offset_sec() const { return tz_offset_sec_; }

private:
    /// Epoch alignment constant: Unix epoch (1970-01-01) was a Thursday.
    /// Sunday is 4 days earlier, so offset = 4 * 86400 seconds.
    /// This makes slot boundaries align with calendar weeks starting on Sunday.
    static constexpr long kEpochSundayOffset = 4L * 86400L;

    /// Compute the scheduling slot for a given time (calendar-based scheduling only).
    ///
    /// Uses floor division to handle negative values correctly:
    /// `val / p - (val % p < 0)` ensures consistent rounding toward negative infinity.
    long Slot(unsigned long time) const {
        long val = static_cast<long>(time) + tz_offset_sec_ + offset_;
        long p = static_cast<long>(period_);
        return val / p - (val % p < 0);
    }

    unsigned long period_;
    long offset_;
    long tz_offset_sec_;  // Timezone offset in seconds (applied in Slot()).
    bool relative_;
    unsigned long start_time_;  // Reference time for relative mode (unsigned, wrap-safe).
    long last_slot_;            // Last computed slot for absolute mode (signed for negative slots).
    unsigned long fire_count_;
    unsigned long missed_ticks_;
    unsigned long max_fires_;
    bool running_;
};

}  // namespace util
}  // namespace foresthub

#endif  // FORESTHUB_UTIL_TICKER_HPP
