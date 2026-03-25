// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "console.hpp"

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <vector>

#include "foresthub/util/strprintf.hpp"

namespace foresthub {
namespace platform {
namespace debug {

DebugConsole::DebugConsole(const std::vector<std::string>& initial_lines) {
    for (const auto& line : initial_lines) {
        input_queue_.push(line);
    }
}

void DebugConsole::Begin() {
    // No-op for debug platform.
}

bool DebugConsole::Available() const noexcept {
    return !input_queue_.empty();
}

char DebugConsole::Read() {
    if (input_queue_.empty()) {
        return -1;
    }
    std::string& front = input_queue_.front();
    char c = front[0];
    front.erase(0, 1);
    if (front.empty()) {
        input_queue_.pop();
    }
    return c;
}

std::string DebugConsole::ReadLine(size_t max_length, unsigned long /*timeout_ms*/, bool /*echo*/) {
    if (input_queue_.empty()) {
        return "";
    }
    std::string line = input_queue_.front();
    input_queue_.pop();
    if (max_length > 0 && line.length() > max_length) {
        line.resize(max_length);
    }
    std::cout << "console: Serial: read \"" << line << "\"" << std::endl;
    return line;
}

Optional<std::string> DebugConsole::TryReadLine(size_t max_length, bool /*echo*/) {
    if (input_queue_.empty()) {
        return Optional<std::string>();
    }
    std::string line = input_queue_.front();
    input_queue_.pop();
    if (max_length > 0 && line.length() > max_length) {
        line.resize(max_length);
    }
    std::cout << "console: Serial: read \"" << line << "\"" << std::endl;
    return Optional<std::string>(std::move(line));
}

void DebugConsole::ClearLineBuffer() {
    // No-op — debug console uses queue, not a line buffer.
}

void DebugConsole::Write(const std::string& data) {
    std::cout << "console: Serial: " << data << std::endl;
}

void DebugConsole::Printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    // Use two-pass vsnprintf for single-allocation formatting.
    va_list args_copy;
    va_copy(args_copy, args);
    int len = std::vsnprintf(nullptr, 0, format, args_copy);
    va_end(args_copy);
    if (len > 0) {
        std::vector<char> buf(static_cast<size_t>(len) + 1);
        std::vsnprintf(buf.data(), buf.size(), format, args);
        std::cout << "console: Serial: " << buf.data() << std::endl;
    }
    va_end(args);
}

void DebugConsole::Flush() noexcept {
    std::cout.flush();
}

}  // namespace debug
}  // namespace platform
}  // namespace foresthub
