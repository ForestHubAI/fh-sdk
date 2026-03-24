// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PLATFORM_PC_CONSOLE_HPP
#define FORESTHUB_PLATFORM_PC_CONSOLE_HPP

#include <string>

#include "foresthub/platform/console.hpp"

namespace foresthub {
namespace platform {
namespace pc {

/// PC console using stdin/stdout for development and testing.
class PcConsole : public Console {
public:
    /// No-op; terminal needs no initialization.
    void Begin() override;

    /// poll()-based check for stdin data.
    bool Available() const noexcept override;
    /// Reads a single character from stdin (blocking, POSIX read).
    char Read() override;
    /// Uses std::getline; timeout_ms and echo parameters are ignored.
    std::string ReadLine(size_t max_length = 256, unsigned long timeout_ms = 0, bool echo = true) override;
    /// Accumulates characters in line_buffer_; returns complete line on Enter.
    Optional<std::string> TryReadLine(size_t max_length = 256, bool echo = true) override;
    /// Discards partially typed input.
    void ClearLineBuffer() override;

    /// Writes to stdout.
    void Write(const std::string& data) override;
    /// Formatted output via vfprintf(stdout, ...).
    void Printf(const char* format, ...) override;
    /// Flushes stdout.
    void Flush() noexcept override;

private:
    std::string line_buffer_;
};

}  // namespace pc
}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_PC_CONSOLE_HPP
