// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifdef FORESTHUB_PLATFORM_PC  // Translation Unit Guard: Only compile on PC

#include "time.hpp"

#include <chrono>
#include <ctime>
#include <thread>

namespace foresthub {
namespace platform {
namespace pc {

PcTime::PcTime() : start_time_(std::chrono::steady_clock::now()) {}

std::string PcTime::SyncTime(const TimeConfig& config) {
    // Store timezone offsets so GetLocalTime() / gmt_offset_sec() reflect them.
    gmt_offset_sec_ = config.gmt_offset_sec;
    daylight_offset_sec_ = config.daylight_offset_sec;
    return "";
}

unsigned long PcTime::GetEpochTime() const {
    auto now = std::chrono::system_clock::now();
    std::chrono::seconds epoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
    return static_cast<unsigned long>(epoch.count());
}

unsigned long PcTime::GetMillis() const {
    auto now = std::chrono::steady_clock::now();
    std::chrono::milliseconds elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
    return static_cast<unsigned long>(elapsed.count());
}

void PcTime::Delay(unsigned long ms) const {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

bool PcTime::IsSynced(unsigned long min_epoch) const {
    // PC system clock is always synced.
    return GetEpochTime() >= min_epoch;
}

void PcTime::SetOffset(long gmt_offset_sec, int daylight_offset_sec) {
    gmt_offset_sec_ = gmt_offset_sec;
    daylight_offset_sec_ = daylight_offset_sec;
}

long PcTime::gmt_offset_sec() const {
    return gmt_offset_sec_ + daylight_offset_sec_;
}

void PcTime::GetLocalTime(struct tm& result) const {
    // Apply stored offset to UTC epoch, then convert to calendar (thread-safe).
    time_t local = static_cast<time_t>(GetEpochTime()) + gmt_offset_sec_ + daylight_offset_sec_;
#ifdef _MSC_VER
    gmtime_s(&result, &local);
#else
    gmtime_r(&local, &result);
#endif
}

}  // namespace pc
}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_PC
