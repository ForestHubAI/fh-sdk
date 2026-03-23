// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "foresthub/util/ticker.hpp"

#include <gtest/gtest.h>

#include <climits>
#include <ctime>

using foresthub::util::Ticker;

// Helper: create epoch for a specific UTC date/time.
static unsigned long MakeEpoch(int year, int month, int day, int hour, int minute, int sec = 0) {
    struct tm t = {};
    t.tm_year = year - 1900;
    t.tm_mon = month - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = sec;
#ifdef _WIN32
    return static_cast<unsigned long>(_mkgmtime(&t));
#else
    return static_cast<unsigned long>(timegm(&t));
#endif
}

// =============================================================================
// Periodic mode
// =============================================================================

TEST(Ticker, DefaultState) {
    auto ticker = Ticker::Periodic(1000);
    EXPECT_FALSE(ticker.running());
    EXPECT_EQ(ticker.fire_count(), 0u);
    EXPECT_EQ(ticker.missed_ticks(), 0u);
    EXPECT_EQ(ticker.period(), 1000u);
}

TEST(Ticker, PeriodicCheckBeforeInterval) {
    auto ticker = Ticker::Periodic(1000);
    ticker.Start(0);
    EXPECT_TRUE(ticker.running());
    EXPECT_FALSE(ticker.Check(500));
    EXPECT_EQ(ticker.fire_count(), 0u);
}

TEST(Ticker, PeriodicCheckAtInterval) {
    auto ticker = Ticker::Periodic(1000);
    ticker.Start(0);
    EXPECT_TRUE(ticker.Check(1000));
    EXPECT_EQ(ticker.fire_count(), 1u);
}

TEST(Ticker, PeriodicFiresRepeatedly) {
    auto ticker = Ticker::Periodic(1000);
    ticker.Start(0);
    EXPECT_TRUE(ticker.Check(1000));
    EXPECT_TRUE(ticker.Check(2000));
    EXPECT_TRUE(ticker.Check(3000));
    EXPECT_EQ(ticker.fire_count(), 3u);
}

TEST(Ticker, PeriodicStop) {
    auto ticker = Ticker::Periodic(1000);
    ticker.Start(0);
    EXPECT_TRUE(ticker.Check(1000));
    ticker.Stop();
    EXPECT_FALSE(ticker.running());
    EXPECT_FALSE(ticker.Check(2000));
}

TEST(Ticker, PeriodicMaxFires) {
    auto ticker = Ticker::Periodic(1000);
    ticker.WithMaxFires(3);
    ticker.Start(0);
    EXPECT_TRUE(ticker.Check(1000));
    EXPECT_TRUE(ticker.Check(2000));
    EXPECT_TRUE(ticker.Check(3000));
    EXPECT_EQ(ticker.fire_count(), 3u);
    EXPECT_FALSE(ticker.running());
    EXPECT_FALSE(ticker.Check(4000));
}

TEST(Ticker, PeriodicMissedTicksZero) {
    auto ticker = Ticker::Periodic(1000);
    ticker.Start(0);
    EXPECT_TRUE(ticker.Check(1000));
    EXPECT_EQ(ticker.missed_ticks(), 0u);
}

TEST(Ticker, PeriodicMissedTicksGap) {
    // period=5000, gap of 37000 → 37000/5000 = 7 full intervals, missed = 7-1 = 6.
    auto ticker = Ticker::Periodic(5000);
    ticker.Start(0);
    EXPECT_TRUE(ticker.Check(37000));
    EXPECT_EQ(ticker.missed_ticks(), 6u);
    EXPECT_EQ(ticker.fire_count(), 1u);
}

TEST(Ticker, PeriodicMissedTicksReset) {
    // After a gap, next on-time check resets missed_ticks to 0.
    auto ticker = Ticker::Periodic(1000);
    ticker.Start(0);
    EXPECT_TRUE(ticker.Check(5000));
    EXPECT_EQ(ticker.missed_ticks(), 4u);
    EXPECT_TRUE(ticker.Check(6000));
    EXPECT_EQ(ticker.missed_ticks(), 0u);
}

TEST(Ticker, PeriodicDriftCorrection) {
    // After gap, next fire snaps to grid, not to "now + period".
    // Start(0), period=5000: grid = 5000, 10000, 15000, ...
    // Check(37000) fires (missed=6), start_time_ snaps to 35000.
    // Check(40000) fires (40000-35000=5000 >= 5000).
    auto ticker = Ticker::Periodic(5000);
    ticker.Start(0);
    EXPECT_TRUE(ticker.Check(37000));
    EXPECT_TRUE(ticker.Check(40000));   // 40000 = 35000 + 5000, fires on grid.
    EXPECT_FALSE(ticker.Check(41000));  // Not yet next grid point (45000).
    EXPECT_TRUE(ticker.Check(45000));   // Next grid point.
}

TEST(Ticker, PeriodicUnsignedOverflow) {
    // Start near ULONG_MAX, check after wrap. Unsigned subtraction is wrap-safe.
    unsigned long near_max = ULONG_MAX - 500;
    auto ticker = Ticker::Periodic(1000);
    ticker.Start(near_max);
    // After wrap: 499 (ULONG_MAX - 500 + 1000 = ULONG_MAX + 500, wraps to 499).
    // Unsigned subtraction: 499 - (ULONG_MAX - 500) = 1000 (correct).
    EXPECT_TRUE(ticker.Check(499));
    EXPECT_EQ(ticker.fire_count(), 1u);
    EXPECT_EQ(ticker.missed_ticks(), 0u);
}

// =============================================================================
// OneShot mode
// =============================================================================

TEST(Ticker, OneShotFiresOnce) {
    auto ticker = Ticker::OneShot(1000);
    ticker.Start(0);
    EXPECT_TRUE(ticker.Check(1000));
    EXPECT_FALSE(ticker.running());
    EXPECT_FALSE(ticker.Check(2000));
    EXPECT_EQ(ticker.fire_count(), 1u);
}

TEST(Ticker, OneShotNotBeforeDelay) {
    auto ticker = Ticker::OneShot(1000);
    ticker.Start(0);
    EXPECT_FALSE(ticker.Check(500));
    EXPECT_FALSE(ticker.Check(999));
    EXPECT_TRUE(ticker.Check(1000));
}

// =============================================================================
// Daily factory
// =============================================================================

TEST(Ticker, DailyMatchingTime) {
    // Daily at 14:00.
    auto ticker = Ticker::Daily(14, 0);
    unsigned long start = MakeEpoch(2026, 2, 25, 13, 59);
    unsigned long check = MakeEpoch(2026, 2, 25, 14, 0);
    ticker.Start(start);
    EXPECT_TRUE(ticker.Check(check));
    EXPECT_EQ(ticker.fire_count(), 1u);
}

TEST(Ticker, DailyNonMatchingTime) {
    // Daily at 14:00. Check at 14:00 same slot as Start → no fire.
    auto ticker = Ticker::Daily(14, 0);
    unsigned long start = MakeEpoch(2026, 2, 25, 14, 0);
    unsigned long check = MakeEpoch(2026, 2, 25, 14, 30);
    ticker.Start(start);
    // Start and check are in the same day-slot → no new slot crossing.
    EXPECT_FALSE(ticker.Check(check));
}

TEST(Ticker, DailyDedupSameMinute) {
    // Multiple checks within the same day-slot fire only once.
    auto ticker = Ticker::Daily(14, 0);
    unsigned long start = MakeEpoch(2026, 2, 25, 13, 59);
    ticker.Start(start);
    EXPECT_TRUE(ticker.Check(MakeEpoch(2026, 2, 25, 14, 0)));
    // Still in the same day-slot.
    EXPECT_FALSE(ticker.Check(MakeEpoch(2026, 2, 25, 14, 30)));
    EXPECT_FALSE(ticker.Check(MakeEpoch(2026, 2, 25, 23, 59)));
}

TEST(Ticker, DailyRepeatsNextDay) {
    auto ticker = Ticker::Daily(14, 0);
    unsigned long start = MakeEpoch(2026, 2, 25, 13, 59);
    ticker.Start(start);
    EXPECT_TRUE(ticker.Check(MakeEpoch(2026, 2, 25, 14, 0)));
    EXPECT_TRUE(ticker.Check(MakeEpoch(2026, 2, 26, 14, 0)));
    EXPECT_EQ(ticker.fire_count(), 2u);
}

// =============================================================================
// Weekly factory
// =============================================================================

TEST(Ticker, WeeklyCorrectDay) {
    // Monday at 08:00. 2026-02-23 is Monday (weekday=1).
    auto ticker = Ticker::Weekly(1, 8, 0);
    unsigned long start = MakeEpoch(2026, 2, 22, 12, 0);  // Sunday.
    ticker.Start(start);
    EXPECT_TRUE(ticker.Check(MakeEpoch(2026, 2, 23, 8, 0)));
    EXPECT_EQ(ticker.fire_count(), 1u);
}

TEST(Ticker, WeeklyWrongDay) {
    // Monday at 08:00. Check on Tuesday → same week-slot → no new fire.
    auto ticker = Ticker::Weekly(1, 8, 0);
    unsigned long start = MakeEpoch(2026, 2, 22, 12, 0);  // Sunday.
    ticker.Start(start);
    // Monday 08:00 fires (new slot).
    EXPECT_TRUE(ticker.Check(MakeEpoch(2026, 2, 23, 8, 0)));
    // Tuesday 08:00 → same week-slot as Monday, no fire.
    EXPECT_FALSE(ticker.Check(MakeEpoch(2026, 2, 24, 8, 0)));
    // Next Monday fires.
    EXPECT_TRUE(ticker.Check(MakeEpoch(2026, 3, 2, 8, 0)));
}

// =============================================================================
// Hourly factory
// =============================================================================

TEST(Ticker, HourlyAtMinute) {
    // Fires at minute 0 of each hour.
    auto ticker = Ticker::Hourly(0);
    unsigned long start = MakeEpoch(2026, 2, 25, 9, 59);
    ticker.Start(start);
    EXPECT_TRUE(ticker.Check(MakeEpoch(2026, 2, 25, 10, 0)));
    EXPECT_EQ(ticker.fire_count(), 1u);
}

TEST(Ticker, HourlyRepeats) {
    auto ticker = Ticker::Hourly(0);
    unsigned long start = MakeEpoch(2026, 2, 25, 9, 59);
    ticker.Start(start);
    EXPECT_TRUE(ticker.Check(MakeEpoch(2026, 2, 25, 10, 0)));
    EXPECT_FALSE(ticker.Check(MakeEpoch(2026, 2, 25, 10, 30)));
    EXPECT_TRUE(ticker.Check(MakeEpoch(2026, 2, 25, 11, 0)));
    EXPECT_EQ(ticker.fire_count(), 2u);
}

// =============================================================================
// Timezone offset tests (WithTimezoneOffset)
// =============================================================================

TEST(Ticker, WithTimezoneOffsetPositive) {
    // Daily at 15:00 local, CET (UTC+1). Raw UTC 14:00 should fire.
    auto ticker = Ticker::Daily(15, 0).WithTimezoneOffset(3600);
    ticker.Start(MakeEpoch(2026, 2, 25, 13, 59));
    EXPECT_TRUE(ticker.Check(MakeEpoch(2026, 2, 25, 14, 0)));
}

TEST(Ticker, WithTimezoneOffsetNegative) {
    // Daily at 15:00 local, UTC-1. Raw UTC 16:00 should fire.
    auto ticker = Ticker::Daily(15, 0).WithTimezoneOffset(-3600);
    ticker.Start(MakeEpoch(2026, 2, 25, 15, 59));
    EXPECT_TRUE(ticker.Check(MakeEpoch(2026, 2, 25, 16, 0)));
}

TEST(Ticker, WithTimezoneOffsetBuilderChaining) {
    // Builder returns Ticker& for chaining.
    auto ticker = Ticker::Daily(14, 0);
    Ticker& ref = ticker.WithTimezoneOffset(3600);
    EXPECT_EQ(&ref, &ticker);
}

TEST(Ticker, TzOffsetSecDefaultZero) {
    auto ticker = Ticker::Daily(14, 0);
    EXPECT_EQ(ticker.tz_offset_sec(), 0);
}

TEST(Ticker, TzOffsetSecReturnsConfiguredValue) {
    auto ticker = Ticker::Daily(14, 0).WithTimezoneOffset(7200);
    EXPECT_EQ(ticker.tz_offset_sec(), 7200);
}

TEST(Ticker, SetTimezoneOffsetRuntime) {
    // Start with CET (+3600), switch to CEST (+7200) mid-run.
    // Daily at 10:00 local.
    auto ticker = Ticker::Daily(10, 0).WithTimezoneOffset(3600);
    ticker.Start(MakeEpoch(2026, 2, 25, 8, 59));

    // UTC 09:00 = CET 10:00 → fires.
    EXPECT_TRUE(ticker.Check(MakeEpoch(2026, 2, 25, 9, 0)));

    // DST transition: switch to CEST (+7200).
    ticker.SetTimezoneOffset(7200);
    EXPECT_EQ(ticker.tz_offset_sec(), 7200);

    // Next day: UTC 08:00 = CEST 10:00 → fires.
    EXPECT_TRUE(ticker.Check(MakeEpoch(2026, 2, 26, 8, 0)));
}

TEST(Ticker, WithTimezoneOffsetWeekly) {
    // Monday at 08:00 local, CET (UTC+1). UTC 07:00 Mon should fire.
    auto ticker = Ticker::Weekly(1, 8, 0).WithTimezoneOffset(3600);
    unsigned long start = MakeEpoch(2026, 2, 22, 12, 0);  // Sunday.
    ticker.Start(start);
    EXPECT_TRUE(ticker.Check(MakeEpoch(2026, 2, 23, 7, 0)));  // Monday 07:00 UTC = 08:00 CET
    EXPECT_EQ(ticker.fire_count(), 1u);
}

TEST(Ticker, TzOffsetIgnoredInRelativeMode) {
    // Periodic mode does not use Slot(), so tz_offset has no effect.
    auto ticker = Ticker::Periodic(1000).WithTimezoneOffset(9999);
    ticker.Start(0);
    EXPECT_TRUE(ticker.Check(1000));
    EXPECT_EQ(ticker.fire_count(), 1u);
}

// =============================================================================
// Builder tests
// =============================================================================

TEST(Ticker, WithMaxFiresChaining) {
    // Builder returns Ticker& for chaining.
    auto ticker = Ticker::Periodic(1000);
    Ticker& ref = ticker.WithMaxFires(5);
    EXPECT_EQ(&ref, &ticker);
}

TEST(Ticker, MaxFiresStopsAllModes) {
    // WithMaxFires works with Daily factory.
    auto ticker = Ticker::Daily(14, 0);
    ticker.WithMaxFires(2);
    unsigned long start = MakeEpoch(2026, 2, 25, 13, 59);
    ticker.Start(start);
    EXPECT_TRUE(ticker.Check(MakeEpoch(2026, 2, 25, 14, 0)));
    EXPECT_TRUE(ticker.Check(MakeEpoch(2026, 2, 26, 14, 0)));
    EXPECT_FALSE(ticker.running());
    EXPECT_FALSE(ticker.Check(MakeEpoch(2026, 2, 27, 14, 0)));
    EXPECT_EQ(ticker.fire_count(), 2u);
}

// =============================================================================
// Raw constructor
// =============================================================================

TEST(Ticker, RawConstructorCustomOffset) {
    // Manual Ticker(period, offset) — absolute mode with custom alignment.
    // period=3600, offset=0 → fires at every full hour boundary of epoch.
    Ticker ticker(3600, 0);
    unsigned long start = MakeEpoch(2026, 2, 25, 9, 59);
    ticker.Start(start);
    EXPECT_TRUE(ticker.Check(MakeEpoch(2026, 2, 25, 10, 0)));
}

// =============================================================================
// missed_ticks for absolute mode
// =============================================================================

TEST(Ticker, DailyMissedTicks) {
    // Start at 14:00 Feb 25 (slot boundary), check at 14:00 Feb 28.
    // Crosses 3 day-slots (Feb 26, 27, 28), fires once, missed 2.
    auto ticker = Ticker::Daily(14, 0);
    ticker.Start(MakeEpoch(2026, 2, 25, 14, 0));
    EXPECT_TRUE(ticker.Check(MakeEpoch(2026, 2, 28, 14, 0)));
    EXPECT_EQ(ticker.missed_ticks(), 2u);
}

TEST(Ticker, HourlyMissedTicks) {
    // Start at 10:00 (slot boundary), check at 13:00.
    // Crosses 3 hour-slots (11:00, 12:00, 13:00), fires once, missed 2.
    auto ticker = Ticker::Hourly(0);
    ticker.Start(MakeEpoch(2026, 2, 25, 10, 0));
    EXPECT_TRUE(ticker.Check(MakeEpoch(2026, 2, 25, 13, 0)));
    EXPECT_EQ(ticker.missed_ticks(), 2u);
}
