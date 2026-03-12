// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

// =============================================================================
// ForestHub Ticker — Comprehensive Test
// =============================================================================
// Mini test-runner that validates ALL Ticker modes on real hardware:
//
//   Phase A: Simulated time (33 tests, instant)
//     - Periodic, OneShot, Daily, Weekly, Hourly
//     - Edge cases: millis() overflow, missed ticks, drift correction, midnight boundary
//     - Timezone offsets, builder chaining, lifecycle (start/stop/restart)
//
//   Phase B: Real hardware time (4 tests, ~20 seconds)
//     - Periodic + OneShot with actual millis()
//     - Absolute short-period with millis()/1000
//
// No WiFi/NTP required. Runs on any board with Console + Time.
//
// Expected output (115200 baud):
//
//   --------------------------------------------------
//     ForestHub Ticker — Comprehensive Test
//   --------------------------------------------------
//   Tests: 33 simulated (Phase A) + 4 real-time (Phase B)
//   ...
//   --------------------------------------------------
//     RESULTS: 37/37 passed
//   --------------------------------------------------
//     ALL PASSED
//
// =============================================================================

#include <Arduino.h>

#include <climits>
#include <cstdarg>

#include "env.hpp"
#include "foresthub/platform/platform.hpp"
#include "foresthub/util/ticker.hpp"

using foresthub::util::Ticker;

// -- Global state -------------------------------------------------------------

static std::shared_ptr<foresthub::platform::PlatformContext> platform;
static int tests_passed = 0;
static int tests_failed = 0;
static int tests_run = 0;

// -- MakeEpoch helper ---------------------------------------------------------

/// Compute Unix epoch seconds from UTC date/time components.
/// Pure integer arithmetic — no timegm(), no <ctime> dependency.
/// Valid for dates from 1970 onwards. Handles leap years correctly.
static unsigned long MakeEpoch(int year, int month, int day, int hour, int minute, int sec = 0) {
    static const int kDaysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    long days = 0;

    // Full years from 1970
    for (int y = 1970; y < year; ++y) {
        bool leap = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
        days += leap ? 366 : 365;
    }

    // Full months in target year
    bool leap_year = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    for (int m = 1; m < month; ++m) {
        days += kDaysInMonth[m];
        if (m == 2 && leap_year) days += 1;
    }

    // Days in target month (1-based)
    days += day - 1;

    return static_cast<unsigned long>(days) * 86400UL + static_cast<unsigned long>(hour) * 3600UL +
           static_cast<unsigned long>(minute) * 60UL + static_cast<unsigned long>(sec);
}

// -- Test framework -----------------------------------------------------------

static void PrintLine() {
    platform->console->Printf("--------------------------------------------------\n");
}

/// Verbose log helper — indented under test name.
static void Log(const char* format, ...) {
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    platform->console->Printf("  %s\n", buf);
}

/// Assert helper — prints [OK] or [FAIL], returns condition.
static bool Expect(bool condition, const char* description) {
    if (condition) {
        platform->console->Printf("    [OK] %s\n", description);
    } else {
        platform->console->Printf("    [FAIL] %s\n", description);
    }
    return condition;
}

/// Run a single test function. Tracks pass/fail counts.
typedef bool (*TestFn)();

static void RunTest(const char* name, TestFn fn) {
    ++tests_run;
    platform->console->Printf("\n[TEST %d] %s\n", tests_run, name);
    bool passed = fn();
    if (passed) {
        ++tests_passed;
        platform->console->Printf("  >> PASS\n");
    } else {
        ++tests_failed;
        platform->console->Printf("  >> FAIL\n");
    }
    platform->console->Flush();
}

// =============================================================================
// Phase A: Simulated Time
// =============================================================================

// -- A0: MakeEpoch validation -------------------------------------------------

static bool TestMakeEpochKnownValue() {
    unsigned long epoch = MakeEpoch(2026, 1, 1, 0, 0, 0);
    Log("MakeEpoch(2026,1,1,0,0,0) = %lu, expected 1767225600", epoch);
    return Expect(epoch == 1767225600UL, "MakeEpoch matches known constant 1767225600");
}

// -- A1: Periodic -------------------------------------------------------------

static bool TestPeriodicDefaultState() {
    auto ticker = Ticker::Periodic(1000);
    Log("Created Periodic(1000), no Start() called");
    bool ok = true;
    ok &= Expect(!ticker.running(), "running() == false");
    ok &= Expect(ticker.fire_count() == 0, "fire_count() == 0");
    ok &= Expect(ticker.missed_ticks() == 0, "missed_ticks() == 0");
    ok &= Expect(ticker.period() == 1000, "period() == 1000");
    return ok;
}

static bool TestPeriodicBeforeInterval() {
    auto ticker = Ticker::Periodic(1000);
    ticker.Start(0);
    Log("Start(0), period=1000");

    bool result = ticker.Check(500);
    Log("Check(500) = %s, fire_count=%lu", result ? "true" : "false", ticker.fire_count());
    bool ok = true;
    ok &= Expect(!result, "Check(500) returns false");
    ok &= Expect(ticker.running(), "still running");
    ok &= Expect(ticker.fire_count() == 0, "fire_count == 0");
    return ok;
}

static bool TestPeriodicAtInterval() {
    auto ticker = Ticker::Periodic(1000);
    ticker.Start(0);
    Log("Start(0), period=1000");

    bool result = ticker.Check(1000);
    Log("Check(1000) = %s, fire_count=%lu", result ? "true" : "false", ticker.fire_count());
    bool ok = true;
    ok &= Expect(result, "Check(1000) returns true");
    ok &= Expect(ticker.fire_count() == 1, "fire_count == 1");
    return ok;
}

static bool TestPeriodicMultipleFires() {
    auto ticker = Ticker::Periodic(1000);
    ticker.Start(0);
    Log("Start(0), period=1000");

    bool r1 = ticker.Check(1000);
    Log("Check(1000) = %s, fire_count=%lu", r1 ? "true" : "false", ticker.fire_count());
    bool r2 = ticker.Check(2000);
    Log("Check(2000) = %s, fire_count=%lu", r2 ? "true" : "false", ticker.fire_count());
    bool r3 = ticker.Check(3000);
    Log("Check(3000) = %s, fire_count=%lu", r3 ? "true" : "false", ticker.fire_count());

    bool ok = true;
    ok &= Expect(r1 && r2 && r3, "all three checks return true");
    ok &= Expect(ticker.fire_count() == 3, "fire_count == 3");
    return ok;
}

static bool TestPeriodicStop() {
    auto ticker = Ticker::Periodic(1000);
    ticker.Start(0);
    ticker.Check(1000);
    Log("Start(0), Check(1000) fired, then Stop()");

    ticker.Stop();
    bool result = ticker.Check(2000);
    Log("Check(2000) after Stop() = %s", result ? "true" : "false");

    bool ok = true;
    ok &= Expect(!ticker.running(), "running() == false");
    ok &= Expect(!result, "Check(2000) returns false after Stop");
    return ok;
}

static bool TestPeriodicMaxFires() {
    auto ticker = Ticker::Periodic(1000);
    ticker.WithMaxFires(3);
    ticker.Start(0);
    Log("Start(0), period=1000, MaxFires(3)");

    ticker.Check(1000);
    Log("Check(1000) fire_count=%lu, running=%s", ticker.fire_count(), ticker.running() ? "true" : "false");
    ticker.Check(2000);
    Log("Check(2000) fire_count=%lu, running=%s", ticker.fire_count(), ticker.running() ? "true" : "false");
    ticker.Check(3000);
    Log("Check(3000) fire_count=%lu, running=%s", ticker.fire_count(), ticker.running() ? "true" : "false");

    bool result = ticker.Check(4000);
    Log("Check(4000) = %s (should be false, auto-stopped)", result ? "true" : "false");

    bool ok = true;
    ok &= Expect(ticker.fire_count() == 3, "fire_count == 3");
    ok &= Expect(!ticker.running(), "running() == false (auto-stopped)");
    ok &= Expect(!result, "Check(4000) returns false");
    return ok;
}

static bool TestPeriodicMissedTicks() {
    auto ticker = Ticker::Periodic(5000);
    ticker.Start(0);
    Log("Start(0), period=5000");

    bool result = ticker.Check(37000);
    Log("Check(37000) = %s, fire_count=%lu, missed=%lu", result ? "true" : "false", ticker.fire_count(),
        ticker.missed_ticks());

    bool ok = true;
    ok &= Expect(result, "Check(37000) returns true");
    ok &= Expect(ticker.missed_ticks() == 6, "missed_ticks == 6 (37000/5000=7 intervals, minus 1)");
    ok &= Expect(ticker.fire_count() == 1, "fire_count == 1 (fires once, reports missed)");
    return ok;
}

static bool TestPeriodicMissedTicksReset() {
    auto ticker = Ticker::Periodic(1000);
    ticker.Start(0);
    Log("Start(0), period=1000");

    ticker.Check(5000);
    Log("Check(5000) missed=%lu", ticker.missed_ticks());
    bool ok = Expect(ticker.missed_ticks() == 4, "missed_ticks == 4 after gap");

    ticker.Check(6000);
    Log("Check(6000) missed=%lu (on-time)", ticker.missed_ticks());
    ok &= Expect(ticker.missed_ticks() == 0, "missed_ticks == 0 after on-time check");
    return ok;
}

static bool TestPeriodicDriftCorrection() {
    auto ticker = Ticker::Periodic(5000);
    ticker.Start(0);
    Log("Start(0), period=5000, grid: 5000, 10000, 15000, ...");

    // Gap: jump to 37000. Fires, snaps start_time_ to 35000.
    ticker.Check(37000);
    Log("Check(37000) fired, drift-corrected to grid 35000");

    bool r1 = ticker.Check(40000);
    Log("Check(40000) = %s (35000+5000=40000, on grid)", r1 ? "true" : "false");
    bool r2 = ticker.Check(41000);
    Log("Check(41000) = %s (not yet 45000)", r2 ? "true" : "false");
    bool r3 = ticker.Check(45000);
    Log("Check(45000) = %s (next grid point)", r3 ? "true" : "false");

    bool ok = true;
    ok &= Expect(r1, "Check(40000) fires (on grid)");
    ok &= Expect(!r2, "Check(41000) does not fire (between grid points)");
    ok &= Expect(r3, "Check(45000) fires (next grid point)");
    return ok;
}

static bool TestPeriodicUnsignedOverflow() {
    unsigned long near_max = ULONG_MAX - 500;
    auto ticker = Ticker::Periodic(1000);
    ticker.Start(near_max);
    Log("Start(%lu), period=1000 (near ULONG_MAX)", near_max);

    // After wrap: ULONG_MAX-500 + 1000 wraps to 499.
    // Unsigned subtraction: 499 - (ULONG_MAX-500) = 1000 (correct, wrap-safe).
    bool result = ticker.Check(499);
    Log("Check(499) = %s [after 32-bit wrap], fire_count=%lu, missed=%lu", result ? "true" : "false",
        ticker.fire_count(), ticker.missed_ticks());

    bool ok = true;
    ok &= Expect(result, "fires after 32-bit unsigned wrap");
    ok &= Expect(ticker.fire_count() == 1, "fire_count == 1");
    ok &= Expect(ticker.missed_ticks() == 0, "missed_ticks == 0");
    return ok;
}

// -- A2: OneShot --------------------------------------------------------------

static bool TestOneShotFires() {
    auto ticker = Ticker::OneShot(1000);
    ticker.Start(0);
    Log("OneShot(1000), Start(0)");

    bool r1 = ticker.Check(1000);
    Log("Check(1000) = %s, running=%s", r1 ? "true" : "false", ticker.running() ? "true" : "false");
    bool r2 = ticker.Check(2000);
    Log("Check(2000) = %s (should not fire again)", r2 ? "true" : "false");

    bool ok = true;
    ok &= Expect(r1, "fires at 1000");
    ok &= Expect(!ticker.running(), "running() == false after firing");
    ok &= Expect(!r2, "does not fire again at 2000");
    ok &= Expect(ticker.fire_count() == 1, "fire_count == 1");
    return ok;
}

static bool TestOneShotNotBefore() {
    auto ticker = Ticker::OneShot(1000);
    ticker.Start(0);
    Log("OneShot(1000), Start(0)");

    bool r1 = ticker.Check(500);
    Log("Check(500) = %s", r1 ? "true" : "false");
    bool r2 = ticker.Check(999);
    Log("Check(999) = %s", r2 ? "true" : "false");
    bool r3 = ticker.Check(1000);
    Log("Check(1000) = %s", r3 ? "true" : "false");

    bool ok = true;
    ok &= Expect(!r1, "Check(500) returns false");
    ok &= Expect(!r2, "Check(999) returns false");
    ok &= Expect(r3, "Check(1000) returns true");
    return ok;
}

// -- A3: Daily ----------------------------------------------------------------

static bool TestDailyMatching() {
    auto ticker = Ticker::Daily(14, 0);
    unsigned long start = MakeEpoch(2026, 2, 25, 13, 59);
    unsigned long check = MakeEpoch(2026, 2, 25, 14, 0);
    ticker.Start(start);
    Log("Daily(14,0), Start=2026-02-25 13:59 (%lu)", start);

    bool result = ticker.Check(check);
    Log("Check(2026-02-25 14:00 = %lu) = %s, fire_count=%lu", check, result ? "true" : "false", ticker.fire_count());

    bool ok = true;
    ok &= Expect(result, "fires at 14:00");
    ok &= Expect(ticker.fire_count() == 1, "fire_count == 1");
    return ok;
}

static bool TestDailySameSlot() {
    auto ticker = Ticker::Daily(14, 0);
    unsigned long start = MakeEpoch(2026, 2, 25, 14, 0);
    unsigned long check = MakeEpoch(2026, 2, 25, 14, 30);
    ticker.Start(start);
    Log("Daily(14,0), Start=2026-02-25 14:00, Check=14:30 (same day-slot)");

    bool result = ticker.Check(check);
    Log("Check(14:30) = %s", result ? "true" : "false");
    return Expect(!result, "no fire within same day-slot");
}

static bool TestDailyDedup() {
    auto ticker = Ticker::Daily(14, 0);
    unsigned long start = MakeEpoch(2026, 2, 25, 13, 59);
    ticker.Start(start);
    Log("Daily(14,0), Start=13:59");

    bool r1 = ticker.Check(MakeEpoch(2026, 2, 25, 14, 0));
    Log("Check(14:00) = %s", r1 ? "true" : "false");
    bool r2 = ticker.Check(MakeEpoch(2026, 2, 25, 14, 30));
    Log("Check(14:30) = %s (same slot)", r2 ? "true" : "false");
    bool r3 = ticker.Check(MakeEpoch(2026, 2, 25, 23, 59));
    Log("Check(23:59) = %s (same slot)", r3 ? "true" : "false");

    bool ok = true;
    ok &= Expect(r1, "fires at 14:00");
    ok &= Expect(!r2, "no fire at 14:30 (dedup)");
    ok &= Expect(!r3, "no fire at 23:59 (dedup)");
    return ok;
}

static bool TestDailyRepeatsNextDay() {
    auto ticker = Ticker::Daily(14, 0);
    ticker.Start(MakeEpoch(2026, 2, 25, 13, 59));
    Log("Daily(14,0), Start=2/25 13:59");

    bool r1 = ticker.Check(MakeEpoch(2026, 2, 25, 14, 0));
    Log("Check(2/25 14:00) = %s", r1 ? "true" : "false");
    bool r2 = ticker.Check(MakeEpoch(2026, 2, 26, 14, 0));
    Log("Check(2/26 14:00) = %s, fire_count=%lu", r2 ? "true" : "false", ticker.fire_count());

    bool ok = true;
    ok &= Expect(r1, "fires day 1");
    ok &= Expect(r2, "fires day 2");
    ok &= Expect(ticker.fire_count() == 2, "fire_count == 2");
    return ok;
}

static bool TestDailyMidnightBoundary() {
    auto ticker = Ticker::Daily(0, 0);
    unsigned long start = MakeEpoch(2026, 2, 25, 23, 59);
    unsigned long check = MakeEpoch(2026, 2, 26, 0, 1);
    ticker.Start(start);
    Log("Daily(0,0), Start=2/25 23:59, Check=2/26 00:01");

    bool result = ticker.Check(check);
    Log("Check(00:01) = %s, missed=%lu", result ? "true" : "false", ticker.missed_ticks());

    bool ok = true;
    ok &= Expect(result, "fires after midnight crossing");
    ok &= Expect(ticker.missed_ticks() == 0, "missed_ticks == 0");
    return ok;
}

static bool TestDailyMissedDays() {
    auto ticker = Ticker::Daily(14, 0);
    ticker.Start(MakeEpoch(2026, 2, 25, 14, 0));
    Log("Daily(14,0), Start=2/25 14:00 (slot boundary)");

    bool result = ticker.Check(MakeEpoch(2026, 2, 28, 14, 0));
    Log("Check(2/28 14:00) = %s, missed=%lu", result ? "true" : "false", ticker.missed_ticks());

    bool ok = true;
    ok &= Expect(result, "fires after 3-day gap");
    ok &= Expect(ticker.missed_ticks() == 2, "missed_ticks == 2 (skipped 2/26 and 2/27)");
    return ok;
}

// -- A4: Weekly ---------------------------------------------------------------

static bool TestWeeklyCorrectDay() {
    // 2026-02-23 is Monday (weekday=1).
    auto ticker = Ticker::Weekly(1, 8, 0);
    unsigned long start = MakeEpoch(2026, 2, 22, 12, 0);  // Sunday
    ticker.Start(start);
    Log("Weekly(1,8,0) = Monday 08:00, Start=Sunday 2/22 12:00");

    bool result = ticker.Check(MakeEpoch(2026, 2, 23, 8, 0));
    Log("Check(Mon 2/23 08:00) = %s, fire_count=%lu", result ? "true" : "false", ticker.fire_count());

    bool ok = true;
    ok &= Expect(result, "fires on Monday");
    ok &= Expect(ticker.fire_count() == 1, "fire_count == 1");
    return ok;
}

static bool TestWeeklyWrongDay() {
    auto ticker = Ticker::Weekly(1, 8, 0);
    ticker.Start(MakeEpoch(2026, 2, 22, 12, 0));  // Sunday
    Log("Weekly(1,8,0), Start=Sunday 2/22");

    bool r1 = ticker.Check(MakeEpoch(2026, 2, 23, 8, 0));  // Monday
    Log("Check(Mon 2/23) = %s", r1 ? "true" : "false");
    bool r2 = ticker.Check(MakeEpoch(2026, 2, 24, 8, 0));  // Tuesday
    Log("Check(Tue 2/24) = %s (same week-slot)", r2 ? "true" : "false");

    bool ok = true;
    ok &= Expect(r1, "fires on Monday");
    ok &= Expect(!r2, "no fire on Tuesday (same week-slot)");
    return ok;
}

static bool TestWeeklyRepeatsNextWeek() {
    auto ticker = Ticker::Weekly(1, 8, 0);
    ticker.Start(MakeEpoch(2026, 2, 22, 12, 0));  // Sunday
    Log("Weekly(1,8,0), Start=Sunday 2/22");

    ticker.Check(MakeEpoch(2026, 2, 23, 8, 0));               // Monday week 1
    bool result = ticker.Check(MakeEpoch(2026, 3, 2, 8, 0));  // Monday week 2
    Log("Check(Mon 3/2) = %s, fire_count=%lu", result ? "true" : "false", ticker.fire_count());

    bool ok = true;
    ok &= Expect(result, "fires next week Monday");
    ok &= Expect(ticker.fire_count() == 2, "fire_count == 2");
    return ok;
}

static bool TestWeeklyMissedWeeks() {
    auto ticker = Ticker::Weekly(1, 8, 0);
    ticker.Start(MakeEpoch(2026, 2, 22, 12, 0));  // Sunday
    Log("Weekly(1,8,0), Start=Sunday 2/22");

    ticker.Check(MakeEpoch(2026, 2, 23, 8, 0));  // Monday 2/23
    Log("Check(Mon 2/23) fired");

    // Jump 3 weeks to Monday 3/16
    bool result = ticker.Check(MakeEpoch(2026, 3, 16, 8, 0));
    Log("Check(Mon 3/16) = %s, missed=%lu", result ? "true" : "false", ticker.missed_ticks());

    bool ok = true;
    ok &= Expect(result, "fires after 3-week gap");
    ok &= Expect(ticker.missed_ticks() == 2, "missed_ticks == 2 (skipped 3/2 and 3/9)");
    return ok;
}

// -- A5: Hourly ---------------------------------------------------------------

static bool TestHourlyAtMinute() {
    auto ticker = Ticker::Hourly(0);
    unsigned long start = MakeEpoch(2026, 2, 25, 9, 59);
    ticker.Start(start);
    Log("Hourly(0), Start=09:59");

    bool result = ticker.Check(MakeEpoch(2026, 2, 25, 10, 0));
    Log("Check(10:00) = %s, fire_count=%lu", result ? "true" : "false", ticker.fire_count());

    bool ok = true;
    ok &= Expect(result, "fires at 10:00");
    ok &= Expect(ticker.fire_count() == 1, "fire_count == 1");
    return ok;
}

static bool TestHourlyRepeats() {
    auto ticker = Ticker::Hourly(0);
    ticker.Start(MakeEpoch(2026, 2, 25, 9, 59));
    Log("Hourly(0), Start=09:59");

    bool r1 = ticker.Check(MakeEpoch(2026, 2, 25, 10, 0));
    Log("Check(10:00) = %s", r1 ? "true" : "false");
    bool r2 = ticker.Check(MakeEpoch(2026, 2, 25, 10, 30));
    Log("Check(10:30) = %s (same hour-slot)", r2 ? "true" : "false");
    bool r3 = ticker.Check(MakeEpoch(2026, 2, 25, 11, 0));
    Log("Check(11:00) = %s", r3 ? "true" : "false");

    bool ok = true;
    ok &= Expect(r1, "fires at 10:00");
    ok &= Expect(!r2, "no fire at 10:30 (same slot)");
    ok &= Expect(r3, "fires at 11:00");
    ok &= Expect(ticker.fire_count() == 2, "fire_count == 2");
    return ok;
}

static bool TestHourlyMissedHours() {
    auto ticker = Ticker::Hourly(0);
    ticker.Start(MakeEpoch(2026, 2, 25, 10, 0));
    Log("Hourly(0), Start=10:00 (slot boundary)");

    bool result = ticker.Check(MakeEpoch(2026, 2, 25, 13, 0));
    Log("Check(13:00) = %s, missed=%lu", result ? "true" : "false", ticker.missed_ticks());

    bool ok = true;
    ok &= Expect(result, "fires after 3-hour gap");
    ok &= Expect(ticker.missed_ticks() == 2, "missed_ticks == 2 (skipped 11:00 and 12:00)");
    return ok;
}

// -- A6: Timezone offsets -----------------------------------------------------

static bool TestPositiveOffset() {
    // Daily at 15:00 local. CET = UTC+3600.
    // User passes epoch + 3600: UTC 14:00 + 3600 = 15:00 local epoch.
    auto ticker = Ticker::Daily(15, 0);
    ticker.Start(MakeEpoch(2026, 2, 25, 13, 59) + 3600);
    Log("Daily(15,0), CET offset +3600");

    bool result = ticker.Check(MakeEpoch(2026, 2, 25, 14, 0) + 3600);
    Log("Check(UTC 14:00 + 3600) = %s", result ? "true" : "false");
    return Expect(result, "fires at 15:00 local (UTC 14:00 + CET offset)");
}

static bool TestNegativeOffset() {
    // Daily at 15:00 local. Offset = -3600.
    // User passes epoch - 3600: UTC 16:00 - 3600 = 15:00 local epoch.
    auto ticker = Ticker::Daily(15, 0);
    ticker.Start(MakeEpoch(2026, 2, 25, 15, 59) - 3600);
    Log("Daily(15,0), offset -3600");

    bool result = ticker.Check(MakeEpoch(2026, 2, 25, 16, 0) - 3600);
    Log("Check(UTC 16:00 - 3600) = %s", result ? "true" : "false");
    return Expect(result, "fires at 15:00 local (UTC 16:00 - offset)");
}

// -- A7: Builder / Lifecycle --------------------------------------------------

static bool TestWithMaxFiresChaining() {
    auto ticker = Ticker::Periodic(1000);
    Ticker* ref = &ticker.WithMaxFires(5);
    Log("WithMaxFires(5) returns &ticker? %s", ref == &ticker ? "yes" : "no");
    return Expect(ref == &ticker, "WithMaxFires returns reference to same object");
}

static bool TestMaxFiresAbsoluteMode() {
    auto ticker = Ticker::Daily(14, 0);
    ticker.WithMaxFires(2);
    ticker.Start(MakeEpoch(2026, 2, 25, 13, 59));
    Log("Daily(14,0).WithMaxFires(2), Start=2/25 13:59");

    bool r1 = ticker.Check(MakeEpoch(2026, 2, 25, 14, 0));
    Log("Check(2/25 14:00) = %s, running=%s", r1 ? "true" : "false", ticker.running() ? "true" : "false");
    bool r2 = ticker.Check(MakeEpoch(2026, 2, 26, 14, 0));
    Log("Check(2/26 14:00) = %s, running=%s", r2 ? "true" : "false", ticker.running() ? "true" : "false");
    bool r3 = ticker.Check(MakeEpoch(2026, 2, 27, 14, 0));
    Log("Check(2/27 14:00) = %s (should be stopped)", r3 ? "true" : "false");

    bool ok = true;
    ok &= Expect(r1, "fires day 1");
    ok &= Expect(r2, "fires day 2");
    ok &= Expect(!ticker.running(), "auto-stopped after 2 fires");
    ok &= Expect(!r3, "no fire day 3");
    ok &= Expect(ticker.fire_count() == 2, "fire_count == 2");
    return ok;
}

static bool TestStartResetsCounters() {
    auto ticker = Ticker::Periodic(1000);
    ticker.Start(0);
    ticker.Check(1000);
    ticker.Check(5000);  // Creates a gap: missed_ticks=3
    Log("After gap: fire_count=%lu, missed=%lu", ticker.fire_count(), ticker.missed_ticks());

    ticker.Stop();
    ticker.Start(10000);
    Log("Stop() + Start(10000): fire_count=%lu, missed=%lu", ticker.fire_count(), ticker.missed_ticks());

    bool ok = true;
    ok &= Expect(ticker.fire_count() == 0, "fire_count reset to 0");
    ok &= Expect(ticker.missed_ticks() == 0, "missed_ticks reset to 0");
    ok &= Expect(ticker.running(), "running() == true after restart");
    return ok;
}

static bool TestCheckWithoutStart() {
    auto ticker = Ticker::Periodic(1000);
    Log("Created Periodic(1000), no Start() — calling Check(1000)");

    bool result = ticker.Check(1000);
    Log("Check(1000) = %s, running=%s", result ? "true" : "false", ticker.running() ? "true" : "false");

    bool ok = true;
    ok &= Expect(!result, "Check returns false without Start");
    ok &= Expect(!ticker.running(), "running() == false");
    return ok;
}

// -- A8: Raw constructor ------------------------------------------------------

static bool TestRawConstructorHourBoundary() {
    // Ticker(3600, 0) in absolute mode: fires at every full-hour epoch boundary.
    Ticker ticker(3600, 0);
    unsigned long start = MakeEpoch(2026, 2, 25, 9, 59);
    ticker.Start(start);
    Log("Ticker(3600, 0), Start=09:59 (%lu)", start);

    bool result = ticker.Check(MakeEpoch(2026, 2, 25, 10, 0));
    Log("Check(10:00) = %s", result ? "true" : "false");
    return Expect(result, "fires at full-hour boundary");
}

// =============================================================================
// Phase B: Real Hardware Time
// =============================================================================

static bool TestRealPeriodic1s() {
    auto ticker = Ticker::Periodic(1000);
    unsigned long start = platform->time->GetMillis();
    ticker.Start(start);
    Log("Start(%lu), period=1000ms, polling for ~3.5 seconds", start);

    int fires = 0;
    while (platform->time->GetMillis() - start < 3500) {
        unsigned long now = platform->time->GetMillis();
        if (ticker.Check(now)) {
            ++fires;
            Log("FIRE #%d at %lu ms (elapsed=%lu, missed=%lu)", fires, now, now - start, ticker.missed_ticks());
        }
        platform->time->Delay(50);
    }
    Log("Total fires: %d (expected 3)", fires);

    bool ok = true;
    ok &= Expect(fires == 3, "fires == 3 in ~3.5 seconds");
    ok &= Expect(ticker.running(), "still running");
    return ok;
}

static bool TestRealPeriodicMissedTicks() {
    auto ticker = Ticker::Periodic(1000);
    unsigned long start = platform->time->GetMillis();
    ticker.Start(start);
    Log("Start(%lu), period=1000ms, blocking for 3000ms...", start);

    platform->time->Delay(3000);
    unsigned long now = platform->time->GetMillis();
    bool result = ticker.Check(now);
    Log("Check(%lu) = %s, elapsed=%lu, fire_count=%lu, missed=%lu", now, result ? "true" : "false", now - start,
        ticker.fire_count(), ticker.missed_ticks());

    bool ok = true;
    ok &= Expect(result, "fires after blocking delay");
    ok &= Expect(ticker.missed_ticks() >= 1, "missed_ticks >= 1");
    Log("(exact missed_ticks depends on Delay() precision)");
    return ok;
}

static bool TestRealOneShot2s() {
    auto ticker = Ticker::OneShot(2000);
    unsigned long start = platform->time->GetMillis();
    ticker.Start(start);
    Log("OneShot(2000), Start(%lu)", start);

    // Should not fire at ~1s
    platform->time->Delay(1000);
    unsigned long now = platform->time->GetMillis();
    bool r1 = ticker.Check(now);
    Log("Check(%lu) at ~1s = %s, running=%s", now, r1 ? "true" : "false", ticker.running() ? "true" : "false");

    // Should fire at ~2.1s
    platform->time->Delay(1100);
    now = platform->time->GetMillis();
    bool r2 = ticker.Check(now);
    Log("Check(%lu) at ~2.1s = %s, running=%s", now, r2 ? "true" : "false", ticker.running() ? "true" : "false");

    // Should not fire again
    platform->time->Delay(500);
    now = platform->time->GetMillis();
    bool r3 = ticker.Check(now);
    Log("Check(%lu) at ~2.6s = %s (should not fire again)", now, r3 ? "true" : "false");

    bool ok = true;
    ok &= Expect(!r1, "does not fire at ~1s");
    ok &= Expect(r2, "fires at ~2.1s");
    ok &= Expect(!ticker.running(), "running() == false after firing");
    ok &= Expect(!r3, "does not fire again");
    return ok;
}

static bool TestRealAbsoluteShortPeriod() {
    // Absolute mode with short period: fires every 3 seconds using millis()/1000.
    Ticker ticker(3, 0);
    unsigned long start_sec = platform->time->GetMillis() / 1000;
    ticker.Start(start_sec);
    Log("Ticker(3, 0), Start(%lu sec), polling for ~7 seconds", start_sec);

    int fires = 0;
    unsigned long start_ms = platform->time->GetMillis();
    while (platform->time->GetMillis() - start_ms < 7000) {
        unsigned long now_sec = platform->time->GetMillis() / 1000;
        if (ticker.Check(now_sec)) {
            ++fires;
            Log("FIRE #%d at %lu sec (missed=%lu)", fires, now_sec, ticker.missed_ticks());
        }
        platform->time->Delay(100);
    }
    Log("Total fires: %d (expected 1-2, depends on start alignment)", fires);

    bool ok = true;
    ok &= Expect(fires >= 1, "at least 1 fire in 7 seconds");
    ok &= Expect(fires <= 3, "at most 3 fires in 7 seconds");
    return ok;
}

// =============================================================================
// Entry point
// =============================================================================

void setup() {
    foresthub::platform::PlatformConfig config;
    platform = foresthub::platform::CreatePlatform(config);
    if (!platform) {
        while (true) {
        }
    }

    platform->console->Begin();
    platform->time->Delay(500);  // Let serial stabilize

    PrintLine();
    platform->console->Printf("  ForestHub Ticker — Comprehensive Test\n");
    PrintLine();
    platform->console->Printf("\nTests: 33 simulated (Phase A) + 4 real-time (Phase B)\n");
    platform->console->Printf("No WiFi/NTP required.\n");
    platform->console->Flush();

    // =========================================================================
    // Phase A: Simulated Time
    // =========================================================================
    platform->console->Printf("\n========== PHASE A: Simulated Time ==========\n");
    platform->console->Flush();

    // A0: MakeEpoch validation
    RunTest("MakeEpoch known value (2026-01-01 == 1767225600)", TestMakeEpochKnownValue);

    // A1: Periodic
    RunTest("Periodic: default state", TestPeriodicDefaultState);
    RunTest("Periodic: check before interval", TestPeriodicBeforeInterval);
    RunTest("Periodic: check at exact interval", TestPeriodicAtInterval);
    RunTest("Periodic: multiple fires", TestPeriodicMultipleFires);
    RunTest("Periodic: stop prevents firing", TestPeriodicStop);
    RunTest("Periodic: MaxFires auto-stop", TestPeriodicMaxFires);
    RunTest("Periodic: missed ticks from gap", TestPeriodicMissedTicks);
    RunTest("Periodic: missed ticks reset", TestPeriodicMissedTicksReset);
    RunTest("Periodic: drift correction snaps to grid", TestPeriodicDriftCorrection);
    RunTest("Periodic: unsigned 32-bit overflow", TestPeriodicUnsignedOverflow);

    // A2: OneShot
    RunTest("OneShot: fires exactly once", TestOneShotFires);
    RunTest("OneShot: not before delay", TestOneShotNotBefore);

    // A3: Daily
    RunTest("Daily: matching hour:minute", TestDailyMatching);
    RunTest("Daily: same day-slot no fire", TestDailySameSlot);
    RunTest("Daily: dedup same slot", TestDailyDedup);
    RunTest("Daily: repeats next day", TestDailyRepeatsNextDay);
    RunTest("Daily: midnight boundary", TestDailyMidnightBoundary);
    RunTest("Daily: missed days (3-day gap)", TestDailyMissedDays);

    // A4: Weekly
    RunTest("Weekly: correct weekday", TestWeeklyCorrectDay);
    RunTest("Weekly: wrong day same slot", TestWeeklyWrongDay);
    RunTest("Weekly: repeats next week", TestWeeklyRepeatsNextWeek);
    RunTest("Weekly: missed weeks (3-week gap)", TestWeeklyMissedWeeks);

    // A5: Hourly
    RunTest("Hourly: at correct minute", TestHourlyAtMinute);
    RunTest("Hourly: repeats each hour", TestHourlyRepeats);
    RunTest("Hourly: missed hours", TestHourlyMissedHours);

    // A6: Timezone
    RunTest("Timezone: positive offset (CET +3600)", TestPositiveOffset);
    RunTest("Timezone: negative offset (-3600)", TestNegativeOffset);

    // A7: Builder/Lifecycle
    RunTest("Builder: WithMaxFires returns reference", TestWithMaxFiresChaining);
    RunTest("Builder: MaxFires with Daily", TestMaxFiresAbsoluteMode);
    RunTest("Lifecycle: Start resets counters", TestStartResetsCounters);
    RunTest("Lifecycle: Check without Start", TestCheckWithoutStart);

    // A8: Raw constructor
    RunTest("Raw constructor: hour boundary", TestRawConstructorHourBoundary);

    // =========================================================================
    // Phase B: Real Hardware Time
    // =========================================================================
    platform->console->Printf("\n========== PHASE B: Real Hardware Time ==========\n");
    platform->console->Printf("(This phase takes ~20 seconds)\n");
    platform->console->Flush();

    RunTest("Real: Periodic 1s for 3.5s", TestRealPeriodic1s);
    RunTest("Real: Periodic missed ticks (3s block)", TestRealPeriodicMissedTicks);
    RunTest("Real: OneShot 2s", TestRealOneShot2s);
    RunTest("Real: Absolute short-period (3s)", TestRealAbsoluteShortPeriod);

    // =========================================================================
    // Summary
    // =========================================================================
    platform->console->Printf("\n");
    PrintLine();
    platform->console->Printf("  RESULTS: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0) {
        platform->console->Printf(" (%d FAILED)", tests_failed);
    }
    platform->console->Printf("\n");
    PrintLine();
    platform->console->Printf("\n  %s\n\n", tests_failed == 0 ? "ALL PASSED" : "SOME TESTS FAILED");
    platform->console->Flush();
}

void loop() {
    platform->time->Delay(10000);
}
