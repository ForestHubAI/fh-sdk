#ifdef FORESTHUB_PLATFORM_ARDUINO  // Translation Unit Guard: Only compile on Arduino

#include "console.hpp"

#include <cstdarg>
#include <cstring>

namespace foresthub {
namespace platform {
namespace arduino {

// =============================================================================
// Configuration Constants
// =============================================================================

// Maximum size for printf output. Larger outputs are truncated with a warning.
// 4KB is a reasonable limit for embedded systems while supporting most use cases.
static constexpr size_t kMaxFormattedLength = 4096;

// Size of the fallback buffer used when dynamic allocation fails.
// 512 bytes provides basic functionality even under memory pressure.
static constexpr size_t kFallbackBufferSize = 512;

// Chunk size for serial output. Prevents hardware buffer overflow on most boards.
// 256 bytes matches typical UART FIFO sizes (ESP32: 128, ATmega: 64, with margin).
static constexpr size_t kChunkSize = 256;

// Timeout for waiting on USB Serial connection during begin() (milliseconds).
// 2.5 seconds is long enough for most USB enumeration but won't block forever
// on battery-powered devices without a host connection.
static constexpr unsigned long kSerialReadyTimeoutMs = 2500;

// =============================================================================
// Construction
// =============================================================================

ArduinoConsole::ArduinoConsole(unsigned long baud_rate) : baud_rate_(baud_rate), io_(&Serial) {}

ArduinoConsole::ArduinoConsole(Stream& io, unsigned long baud_rate) : baud_rate_(baud_rate), io_(&io) {}

// =============================================================================
// Initialization
// =============================================================================

void ArduinoConsole::Begin() {
    // Only initialize the default Serial object here.
    // Other streams (Serial1, Serial2) must be initialized externally because:
    // 1. Stream base class has no begin() method
    // 2. Different hardware UARTs may need different configuration
    if (io_ == &Serial) {
        Serial.begin(baud_rate_);

        // Wait for USB CDC serial to be ready (native USB boards like Leonardo, ESP32-S3).
        // Without this wait, early printf() calls would be lost.
        // Timeout prevents infinite blocking on battery-powered devices without USB host.
        const unsigned long start = millis();
        while (!Serial && (millis() - start < kSerialReadyTimeoutMs)) {
            yield();  // Allow background tasks (WiFi, etc.) to run
        }
    }
}

// =============================================================================
// Input Methods
// =============================================================================

bool ArduinoConsole::Available() const noexcept {
    return io_->available() > 0;
}

char ArduinoConsole::Read() {
    return static_cast<char>(io_->read());
}

std::string ArduinoConsole::ReadLine(size_t max_length, unsigned long timeout_ms, bool echo) {
    std::string result;
    const unsigned long start = millis();

    // Pre-allocate if max_length is known (avoids repeated allocations)
    if (max_length > 0 && max_length <= 1024) {
        result.reserve(max_length);
    }

    for (;;) {
        // Check timeout (if enabled)
        if (timeout_ms > 0) {
            const unsigned long elapsed = millis() - start;
            if (elapsed >= timeout_ms) {
                return "";  // Timeout - return empty string
            }
        }

        // Process available input
        if (io_->available()) {
            const char c = static_cast<char>(io_->read());

            // --- Line Ending Detection ---
            // Handle CR (\r), LF (\n), CRLF (\r\n), and LFCR (\n\r) uniformly.
            // This ensures compatibility with Windows (CRLF), Unix (LF), and old Mac (CR).
            if (c == '\r' || c == '\n') {
                // Consume the paired character if present (e.g., \n after \r)
                if (io_->available()) {
                    const int peeked = io_->peek();
                    if ((c == '\r' && peeked == '\n') || (c == '\n' && peeked == '\r')) {
                        io_->read();  // Discard the second character of the pair
                    }
                }
                if (echo) {
                    io_->println();  // Move cursor to next line
                }
                return result;
            }

            // --- Backspace/Delete Handling ---
            // ASCII 8 = Backspace (Ctrl+H), ASCII 127 = Delete (some terminals send this for backspace)
            constexpr char kBackspace = 8;
            constexpr char kDeleteChar = 127;
            if (c == kBackspace || c == kDeleteChar) {
                if (!result.empty()) {
                    result.pop_back();
                    if (echo) {
                        // Terminal escape: move back, overwrite with space, move back again
                        io_->print("\b \b");
                    }
                }
                continue;
            }

            // --- Printable Character Handling ---
            if (IsPrintableChar(c)) {
                // Only append if within length limit (0 = unlimited)
                if (max_length == 0 || result.length() < max_length) {
                    result += c;
                    if (echo) {
                        io_->print(c);
                    }
                }
                // Characters beyond max_length are silently ignored
            }
        }

        // --- System Responsiveness ---
        // yield() allows RTOS tasks and WiFi stack to run (prevents WDT reset on ESP32)
        yield();
        // Small delay prevents CPU spinning at 100% on fast MCUs with USB CDC
        delay(1);
    }
}

// =============================================================================
// Output Methods
// =============================================================================

void ArduinoConsole::Write(const std::string& data) {
    // Direct output without chunking - strings are typically small.
    // For large output, use printf() which handles chunking automatically.
    io_->print(data.c_str());
}

void ArduinoConsole::Printf(const char* format, ...) {
    va_list args;

    // --- Step 1: Determine required buffer size ---
    // vsnprintf with nullptr returns the number of characters that would be written
    // (excluding null terminator). This is standard C99/C++11 behavior.
    va_start(args, format);
    const int required_len = vsnprintf(nullptr, 0, format, args);
    va_end(args);

    // Nothing to print or format error
    if (required_len <= 0) {
        return;
    }

    // --- Step 2: Apply size limit ---
    size_t len = static_cast<size_t>(required_len);
    bool was_truncated = false;

    if (len > kMaxFormattedLength) {
        len = kMaxFormattedLength;
        was_truncated = true;
    }

    // --- Step 3: Format and output ---
    // Three-tier allocation strategy to minimize heap usage on MCUs:
    //   1. Stack path  (len <= 512): zero heap overhead, covers most printf calls
    //   2. Heap path   (len > 512):  malloc for large outputs
    //   3. Fallback    (malloc fail): truncated stack buffer under memory pressure
    if (len <= kFallbackBufferSize) {
        // Stack path: no heap allocation for small outputs (common case)
        char stack_buf[kFallbackBufferSize];

        va_start(args, format);
        vsnprintf(stack_buf, len + 1, format, args);
        va_end(args);

        for (size_t offset = 0; offset < len; offset += kChunkSize) {
            const size_t chunk_len = (offset + kChunkSize <= len) ? kChunkSize : (len - offset);
            io_->write(reinterpret_cast<const uint8_t*>(stack_buf + offset),
                       chunk_len);  // NOLINT -- Arduino API requires uint8_t*
        }

        if (was_truncated) {
            io_->println("\n[ArduinoConsole::printf] Warning: Output truncated (exceeded 4KB limit).");
        }
    } else {
        // Heap path: malloc for large outputs (malloc returns nullptr on failure; new would terminate)
        auto* buf = static_cast<char*>(malloc(len + 1));

        if (buf != nullptr) {
            va_start(args, format);
            vsnprintf(buf, len + 1, format, args);
            va_end(args);

            for (size_t offset = 0; offset < len; offset += kChunkSize) {
                const size_t chunk_len = (offset + kChunkSize <= len) ? kChunkSize : (len - offset);
                io_->write(reinterpret_cast<const uint8_t*>(buf + offset),
                           chunk_len);  // NOLINT -- Arduino API requires uint8_t*
            }

            if (was_truncated) {
                io_->println("\n[ArduinoConsole::printf] Warning: Output truncated (exceeded 4KB limit).");
            }

            free(buf);
        } else {
            // Fallback: stack buffer when heap is exhausted
            char fallback[kFallbackBufferSize];

            va_start(args, format);
            vsnprintf(fallback, sizeof(fallback), format, args);
            va_end(args);
            fallback[kFallbackBufferSize - 1] = '\0';

            const size_t fallback_len = strlen(fallback);
            for (size_t offset = 0; offset < fallback_len; offset += kChunkSize) {
                const size_t chunk_len = (offset + kChunkSize <= fallback_len) ? kChunkSize : (fallback_len - offset);
                io_->write(reinterpret_cast<const uint8_t*>(fallback + offset),
                           chunk_len);  // NOLINT -- Arduino API requires uint8_t*
            }

            io_->println("\n[ArduinoConsole::printf] Warning: Using fallback buffer (malloc failed).");
        }
    }
}

void ArduinoConsole::Flush() noexcept {
    // Serial.flush() waits until all outgoing data has been transmitted.
    // This is important before sleep modes or when timing-critical output is needed.
    io_->flush();
}

}  // namespace arduino
}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_ARDUINO
