// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "time.hpp"

#include <chrono>
#include <ctime>
#include <thread>

namespace foresthub {
namespace hal {
namespace pc {

PcTime::PcTime() : start_time_(std::chrono::steady_clock::now()) {}

std::string PcTime::SyncTime(const TimeConfig& config) {
    // Store timezone offsets so GetLocalTime() / utc_offset_sec() reflect them.
    std_offset_sec_ = config.std_offset_sec;
    dst_offset_sec_ = config.dst_offset_sec;
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

void PcTime::SetOffset(long std_offset_sec, int dst_offset_sec) {
    std_offset_sec_ = std_offset_sec;
    dst_offset_sec_ = dst_offset_sec;
}

long PcTime::utc_offset_sec() const {
    return std_offset_sec_ + dst_offset_sec_;
}

void PcTime::GetLocalTime(struct tm& result) const {
    // Apply stored offset to UTC epoch, then convert to calendar (thread-safe).
    time_t local = static_cast<time_t>(GetEpochTime()) + utc_offset_sec();
#ifdef _MSC_VER
    gmtime_s(&result, &local);
#else
    gmtime_r(&local, &result);
#endif
    result.tm_isdst = (dst_offset_sec_ != 0) ? 1 : 0;
}

}  // namespace pc
}  // namespace hal
}  // namespace foresthub
