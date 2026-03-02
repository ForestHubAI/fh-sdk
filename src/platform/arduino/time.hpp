#ifndef FORESTHUB_PLATFORM_ARDUINO_TIME_HPP
#define FORESTHUB_PLATFORM_ARDUINO_TIME_HPP

#include "foresthub/platform/time.hpp"

namespace foresthub {
namespace platform {
namespace arduino {

/// TimeInterface implementation using NTP sync, millis() counter, and WDT-safe delay.
class ArduinoTime : public TimeInterface {
public:
    ArduinoTime() = default;

    /// Retries up to 3 times; returns immediately if already synced.
    std::string SyncTime(const TimeConfig& config = {}) override;

    /// Returns time(nullptr) as unsigned long.
    unsigned long GetEpochTime() const override;

    /// Wraps Arduino millis(). Overflows after ~49.7 days.
    unsigned long GetMillis() const override;

    /// Split into 10ms steps with yield() to prevent WDT resets.
    void Delay(unsigned long ms) const override;

    /// Checks if epoch time exceeds min_epoch threshold.
    bool IsSynced(unsigned long min_epoch = 1767225600) const override;

    /// ESP32: calls configTime(). Other boards: stores offset only.
    void SetOffset(long gmt_offset_sec, int daylight_offset_sec) override;
    /// Returns stored total offset (GMT + daylight).
    long gmt_offset_sec() const override;
    /// Computes local time via gmtime(epoch + total_offset).
    void GetLocalTime(struct tm& result) const override;

private:
    long gmt_offset_sec_ = 0;
    int daylight_offset_sec_ = 0;
#if defined(ARDUINO_ARCH_ESP32)
    const char* last_server_ = "pool.ntp.org";
#endif
};

}  // namespace arduino
}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_ARDUINO_TIME_HPP
