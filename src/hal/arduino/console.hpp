// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PLATFORM_ARDUINO_CONSOLE_HPP
#define FORESTHUB_PLATFORM_ARDUINO_CONSOLE_HPP

#include "board_core.hpp"
#include "foresthub/hal/console.hpp"

namespace foresthub {
namespace hal {
namespace arduino {

/// Console implementation for Arduino with line editing, output chunking, and configurable Stream.
class ArduinoConsole : public Console {
public:
    /// Construct using the default Serial port with the given baud rate.
    explicit ArduinoConsole(unsigned long baud_rate = 115200);

    /// Construct with a configurable Stream and baud rate.
    ArduinoConsole(Stream& io, unsigned long baud_rate = 115200);

    /// Calls Serial.begin() with stored baud rate and waits up to 2.5s for USB connection.
    void Begin() override;

    /// Returns true if Serial has data in its receive buffer.
    bool Available() const noexcept override;
    /// Reads one byte from Serial (blocks if no data available).
    char Read() override;

    /// Adds backspace editing and CR/LF/CRLF normalization.
    std::string ReadLine(size_t max_length = 256, unsigned long timeout_ms = 0, bool echo = true) override;

    /// Accumulates available Serial characters; returns complete line on Enter.
    util::Optional<std::string> TryReadLine(size_t max_length = 256, bool echo = true) override;
    /// Discards partially typed input.
    void ClearLineBuffer() override;

    /// Writes data to Serial byte-by-byte.
    void Write(const std::string& data) override;

    /// Chunks large output (up to 4KB) to prevent serial buffer overflow.
    void Printf(const char* format, ...) override;

    /// Waits until all outgoing Serial data has been transmitted.
    void Flush() noexcept override;

private:
    unsigned long baud_rate_;  ///< Communication speed for Begin().
    Stream* io_;               ///< Pointer to the underlying Stream (Serial, Serial1, etc.)
    std::string line_buffer_;  ///< Internal buffer for TryReadLine() character accumulation.

    /// Check if a character is printable ASCII (space through tilde).
    static constexpr bool IsPrintableChar(char c) noexcept { return c >= 32 && c != 127; }
};

}  // namespace arduino
}  // namespace hal
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_ARDUINO_CONSOLE_HPP
