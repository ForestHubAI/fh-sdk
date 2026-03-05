#ifdef FORESTHUB_PLATFORM_PC  // Translation Unit Guard: Only compile on PC

#include "console.hpp"

#include <poll.h>
#include <unistd.h>

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
    struct pollfd pfd = {0, POLLIN, 0};  // fd 0 = stdin
    return poll(&pfd, 1, 0) > 0;
}

char PcConsole::Read() {
    char c = 0;
    ::read(0, &c, 1);  // POSIX unbuffered read on stdin
    return c;
}

Optional<std::string> PcConsole::TryReadLine(size_t max_length, bool /*echo*/) {
    // Echo parameter ignored — terminal handles echo in canonical mode.
    while (Available()) {
        char c = 0;
        ssize_t n = ::read(0, &c, 1);
        if (n <= 0) {
            return Optional<std::string>();
        }
        if (c == '\n') {
            std::string result = line_buffer_;
            line_buffer_.clear();
            if (max_length > 0 && result.length() > max_length) {
                result.resize(max_length);
            }
            return Optional<std::string>(std::move(result));
        }
        if (max_length == 0 || line_buffer_.length() < max_length) {
            line_buffer_ += c;
        }
    }
    return Optional<std::string>();
}

void PcConsole::ClearLineBuffer() {
    line_buffer_.clear();
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
