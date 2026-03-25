// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "console.hpp"

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#include <windows.h>
#else
#include <poll.h>
#include <unistd.h>
#endif

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <string>

namespace foresthub {
namespace hal {
namespace pc {

void PcConsole::Begin() {
    // No-op on PC. stdin/stdout are always available.
}

bool PcConsole::Available() const noexcept {
#ifdef _WIN32
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD events = 0;
    return GetNumberOfConsoleInputEvents(hStdin, &events) && events > 0;
#else
    struct pollfd pfd = {0, POLLIN, 0};  // fd 0 = stdin
    return poll(&pfd, 1, 0) > 0;
#endif
}

char PcConsole::Read() {
    char c = 0;
#ifdef _WIN32
    DWORD bytesRead = 0;
    ReadFile(GetStdHandle(STD_INPUT_HANDLE), &c, 1, &bytesRead, nullptr);
#else
    ::read(0, &c, 1);  // POSIX unbuffered read on stdin
#endif
    return c;
}

Optional<std::string> PcConsole::TryReadLine(size_t max_length, bool /*echo*/) {
    // Echo parameter ignored — terminal handles echo in canonical mode.
    while (Available()) {
        char c = 0;
#ifdef _WIN32
        DWORD n = 0;
        ReadFile(GetStdHandle(STD_INPUT_HANDLE), &c, 1, &n, nullptr);
        if (n <= 0) {
#else
        ssize_t n = ::read(0, &c, 1);
        if (n <= 0) {
#endif
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
}  // namespace hal
}  // namespace foresthub
