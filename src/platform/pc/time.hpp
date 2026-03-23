// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PLATFORM_PC_TIME_HPP
#define FORESTHUB_PLATFORM_PC_TIME_HPP

#include <chrono>

#include "foresthub/platform/time.hpp"

namespace foresthub {
namespace platform {
namespace pc {

/// Time implementation for PC using std::chrono.
class PcTime : public TimeInterface {
public:
    /// Initialize with current steady clock baseline.
    PcTime();

    /// No-op; system clock is always available.
    std::string SyncTime(const TimeConfig& config = {}) override;

    /// Returns std::time(nullptr) as unsigned long.
    unsigned long GetEpochTime() const override;
    /// Returns milliseconds since construction using steady_clock.
    unsigned long GetMillis() const override;
    /// Sleeps via std::this_thread::sleep_for.
    void Delay(unsigned long ms) const override;
    /// Always returns true; system clock is always synced.
    bool IsSynced(unsigned long min_epoch = 1767225600) const override;

    /// Stores offset locally (PC system timezone is unaffected).
    void SetOffset(long std_offset_sec, int dst_offset_sec) override;
    /// Returns stored total offset (standard + DST).
    long utc_offset_sec() const override;
    /// Computes local time via gmtime(epoch + total_offset).
    void GetLocalTime(struct tm& result) const override;

private:
    std::chrono::steady_clock::time_point start_time_;  ///< Baseline for GetMillis() calculation.
    long std_offset_sec_ = 0;
    int dst_offset_sec_ = 0;
};

}  // namespace pc
}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_PC_TIME_HPP
