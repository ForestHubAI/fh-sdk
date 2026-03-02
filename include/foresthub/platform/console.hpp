#ifndef FORESTHUB_PLATFORM_CONSOLE_HPP
#define FORESTHUB_PLATFORM_CONSOLE_HPP

#include <cstdarg>
#include <string>

namespace foresthub {
namespace platform {

/// Abstract interface for console I/O.
///
/// Provides character and line-based input/output for user interaction.
/// Platform implementations handle the underlying I/O transport.
class ConsoleInterface {
public:
    virtual ~ConsoleInterface() = default;

    /// Initialize the console for I/O operations.
    /// Communication speed is configured at construction time via PlatformConfig.
    virtual void Begin() = 0;

    /// Check if input data is available for reading.
    /// @return true if at least one character can be read without blocking.
    virtual bool Available() const noexcept = 0;

    /// Read a single character.
    /// Behavior when no data available is implementation-defined (may block or return -1).
    /// @return The character read, or -1 if no data is available (implementation-defined).
    virtual char Read() = 0;

    /// Read a line of input with optional timeout.
    /// @param max_length Maximum characters to accept (0 = implementation-defined limit).
    /// @param timeout_ms Timeout in milliseconds (0 = wait indefinitely).
    /// @param echo If true, echo characters back to output.
    /// @return The line read (without line ending), or empty string on timeout.
    virtual std::string ReadLine(size_t max_length = 256, unsigned long timeout_ms = 0, bool echo = true) = 0;

    /// Write a string to the console.
    virtual void Write(const std::string& data) = 0;

    /// Formatted output (printf-style).
    /// @param format Printf-style format string.
    /// @param ... Variable arguments matching format specifiers.
#if defined(__GNUC__) || defined(__clang__)
    // GCC/Clang: Enable compile-time format string checking.
    // Argument indices: 2 = format string (1 is implicit 'this'), 3 = first vararg
    virtual void Printf(const char* format, ...) __attribute__((format(printf, 2, 3))) = 0;
#else
    virtual void Printf(const char* format, ...) = 0;
#endif

    /// Flush the output buffer (ensure all pending output is transmitted).
    virtual void Flush() noexcept = 0;
};

}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_CONSOLE_HPP
