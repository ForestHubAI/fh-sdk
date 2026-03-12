// =============================================================================
// ForestHub Embedded Blink Example
// =============================================================================
// Blinks the built-in LED and prints status messages to the console.
// Uses only the ForestHub HAL (GPIO, Console, Time) — no direct Arduino calls.
//
// Build: pio run -d examples/embedded/utility/blink -e esp32dev
//        pio run -d examples/embedded/utility/blink -e portenta_h7_m7
// =============================================================================

#include <Arduino.h>

#include "foresthub/platform/platform.hpp"

// -- Board-specific LED configuration ----------------------------------------
// ESP32:      GPIO2 = built-in blue LED (active HIGH)
// Portenta:   LEDB  = built-in blue LED (active LOW, accent LED on RGB)
#if defined(ARDUINO_PORTENTA_H7_M7)
static constexpr foresthub::platform::PinId kLedPin = LEDB;
static constexpr bool kLedActiveLow = true;
#else
static constexpr foresthub::platform::PinId kLedPin = 2;
static constexpr bool kLedActiveLow = false;
#endif

static constexpr unsigned long kBlinkIntervalMs = 2000;  // Blink every 2 seconds

static int LedOn() {
    return kLedActiveLow ? 0 : 1;
}
static int LedOff() {
    return kLedActiveLow ? 1 : 0;
}

// -- Platform context (file-scope so loop() can access it) --------------------
static std::shared_ptr<foresthub::platform::PlatformContext> platform;

void setup() {
    // 1. Create platform (no WiFi needed — leave network config empty)
    foresthub::platform::PlatformConfig config;
    platform = foresthub::platform::CreatePlatform(config);

    // 2. Initialize console
    platform->console->Begin();
    platform->console->Printf("\n=== ForestHub Blink Example ===\n");
    platform->console->Printf("Using HAL only — no direct Arduino calls\n");
    platform->console->Printf("LED pin: %lu  |  active-low: %s\n\n", static_cast<unsigned long>(kLedPin),
                              kLedActiveLow ? "yes" : "no");

    // 3. Configure LED pin as output
    platform->gpio->SetPinMode(kLedPin, foresthub::platform::PinMode::kOutput);
}

void loop() {
    static int blink_count = 0;
    ++blink_count;

    // LED on
    platform->gpio->DigitalWrite(kLedPin, LedOn());
    platform->console->Printf("Blink #%d — LED ON\n", blink_count);
    platform->time->Delay(kBlinkIntervalMs);

    // LED off
    platform->gpio->DigitalWrite(kLedPin, LedOff());
    platform->console->Printf("Blink #%d — LED OFF\n", blink_count);
    platform->time->Delay(kBlinkIntervalMs);
}
