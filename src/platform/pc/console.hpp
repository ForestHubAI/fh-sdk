#ifndef FORESTHUB_PLATFORM_PC_CONSOLE_HPP
#define FORESTHUB_PLATFORM_PC_CONSOLE_HPP

#include "foresthub/platform/console.hpp"

namespace foresthub {
namespace platform {
namespace pc {

/// PC console using stdin/stdout for development and testing.
class PcConsole : public ConsoleInterface {
public:
    /// No-op; terminal needs no initialization.
    void Begin() override;

    /// Always returns false; PC uses blocking ReadLine() instead.
    bool Available() const noexcept override;
    /// Reads a single character from stdin (blocking).
    char Read() override;
    /// Uses std::getline; timeout_ms and echo parameters are ignored.
    std::string ReadLine(size_t max_length = 256, unsigned long timeout_ms = 0, bool echo = true) override;

    /// Writes to stdout.
    void Write(const std::string& data) override;
    /// Formatted output via vfprintf(stdout, ...).
    void Printf(const char* format, ...) override;
    /// Flushes stdout.
    void Flush() noexcept override;
};

}  // namespace pc
}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_PC_CONSOLE_HPP
