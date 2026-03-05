// =============================================================================
// ForestHub Console Test (Embedded)
// =============================================================================
// Demonstrates non-blocking console I/O with TryReadLine() and ClearLineBuffer().
// The LED blinks every 1000ms — proving the loop never blocks while you type.
// When you press Enter, the loop iteration count confirms the loop kept running.
// With ReadLine(), the LED would freeze and the count would be 0.
//
// No WiFi or API key required — uses only Console, Time, and GPIO.
//
// Build: pio run -d examples/embedded/console_test -e esp32dev
//        pio run -d examples/embedded/console_test -e portenta_h7_m7
//
// Open serial monitor at 115200 baud, type text, press Enter.
// =============================================================================

#include <Arduino.h>

#include "foresthub/platform/platform.hpp"

// -- Board-specific LED configuration ----------------------------------------
#if defined(ARDUINO_PORTENTA_H7_M7)
static constexpr foresthub::platform::PinId kLedPin = LEDB;
static constexpr bool kLedActiveLow = true;
#else
static constexpr foresthub::platform::PinId kLedPin = 2;
static constexpr bool kLedActiveLow = false;
#endif

static std::shared_ptr<foresthub::platform::PlatformContext> platform;
static bool running = true;
static unsigned long last_blink_ms = 0;
static unsigned long loops = 0;
static bool led_on = false;

void setup() {
    foresthub::platform::PlatformConfig config;
    platform = foresthub::platform::CreatePlatform(config);
    if (!platform) {
        while (true) {
        }
    }

    platform->console->Begin();
    platform->time->Delay(500);

    // Configure LED
    platform->gpio->SetPinMode(kLedPin, foresthub::platform::PinMode::kOutput);
    platform->gpio->DigitalWrite(kLedPin, kLedActiveLow ? 1 : 0);

    platform->console->Printf("\n=== ForestHub Console Test (Embedded) ===\n\n");
    platform->console->Printf("The LED blinks every 1000ms to prove TryReadLine() is non-blocking.\n");
    platform->console->Printf("When you press Enter, the loop iteration count confirms the loop\n");
    platform->console->Printf("kept running. With ReadLine(), the LED would freeze and count = 0.\n\n");
    platform->console->Printf("Commands:\n");
    platform->console->Printf("  <any text>  -> echoed back with loop count\n");
    platform->console->Printf("  'clear'     -> clears partial line buffer\n");
    platform->console->Printf("  'quit'      -> stops the loop\n\n");

    last_blink_ms = platform->time->GetMillis();
}

void loop() {
    if (!running) {
        platform->time->Delay(10000);
        return;
    }

    ++loops;

    // --- Background activity: blink LED every 1000ms ---
    unsigned long now = platform->time->GetMillis();
    if (now - last_blink_ms >= 1000) {
        last_blink_ms += 1000;
        led_on = !led_on;
        int value = led_on ? (kLedActiveLow ? 0 : 1) : (kLedActiveLow ? 1 : 0);
        platform->gpio->DigitalWrite(kLedPin, value);
    }

    // --- Non-blocking input poll ---
    foresthub::Optional<std::string> line = platform->console->TryReadLine();

    if (line.HasValue()) {
        const auto& input = *line;

        if (input == "quit") {
            platform->console->Printf("[INFO] Goodbye! (%lu loops total)\n", loops);
            running = false;
        } else if (input == "clear") {
            platform->console->ClearLineBuffer();
            platform->console->Printf("[INFO] Line buffer cleared. (%lu loops since last input)\n", loops);
            loops = 0;
        } else {
            platform->console->Printf("[ECHO] %s  (%lu loops since last input)\n", input.c_str(), loops);
            loops = 0;
        }
    }

    platform->time->Delay(50);
}
