#ifdef FORESTHUB_PLATFORM_PC  // Translation Unit Guard: Only compile on PC

#include "console.hpp"

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <string>

namespace foresthub {
namespace platform {
namespace pc {

void PcConsole::Begin() {
    // No-op on PC. stdin/stdout are always available.
}

bool PcConsole::Available() const noexcept {
    // PC platform uses blocking ReadLine() for input.
    return false;  // NOLINT -- intentional: PC uses blocking I/O
}

char PcConsole::Read() {
    return static_cast<char>(std::getchar());
}

std::string PcConsole::ReadLine(size_t max_length, unsigned long /*timeout_ms*/, bool /*echo*/) {
    // On PC, echo is always enabled (terminal handles it).
    // Timeout is not implemented for simplicity; std::getline blocks.
    std::string line;
    if (!std::getline(std::cin, line)) {
        return "";
    }
    if (max_length > 0 && line.length() > max_length) {
        line.resize(max_length);
    }
    return line;
}

void PcConsole::Write(const std::string& data) {
    std::cout << data;
}

void PcConsole::Printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::vprintf(format, args);
    va_end(args);
}

void PcConsole::Flush() noexcept {
    std::cout.flush();
}

}  // namespace pc
}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_PC
