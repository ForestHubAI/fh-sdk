// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

// =============================================================================
// ForestHub Console Demo (PC)
// =============================================================================
// Demonstrates non-blocking console I/O with TryReadLine() and ClearLineBuffer().
// The loop counts iterations while waiting for input. When you press Enter, the
// iteration count is shown — proving the loop never blocked. With ReadLine()
// the counter would stay at 0 because the loop would be stuck waiting.
//
// No API key or network required — pure HAL usage.
//
// Build:  cd build && cmake .. && make foresthub_console_test
// Run:    ./foresthub_console_test
// =============================================================================

#include <string>

#include "foresthub/platform/platform.hpp"

int main() {
    auto platform = foresthub::platform::CreatePlatform();
    if (!platform) {
        return 1;
    }
    platform->console->Begin();

    platform->console->Printf("=== ForestHub Console Demo (PC) ===\n\n");
    platform->console->Printf("Type something and press Enter. The loop iteration count proves\n");
    platform->console->Printf("TryReadLine() is non-blocking — the loop kept running while you typed.\n");
    platform->console->Printf("With ReadLine(), the count would be 0 (blocked the entire time).\n\n");
    platform->console->Printf("Commands:\n");
    platform->console->Printf("  <any text>  -> echoed back with loop count\n");
    platform->console->Printf("  'clear'     -> clears partial line buffer\n");
    platform->console->Printf("  'quit'      -> exit\n\n");

    bool running = true;
    unsigned long loops = 0;

    while (running) {
        ++loops;

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

    return 0;
}
