// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PLATFORM_DEBUG_CONSOLE_HPP
#define FORESTHUB_PLATFORM_DEBUG_CONSOLE_HPP

#include <queue>
#include <string>
#include <vector>

#include "foresthub/hal/console.hpp"

namespace foresthub {
namespace hal {
namespace debug {

/// Debug console backed by a pre-loaded input queue.
/// All output is prefixed with "console: Serial: " for the Go backend protocol.
class DebugConsole : public Console {
public:
    /// @param initial_lines Lines to pre-load into the input queue (FIFO order).
    explicit DebugConsole(const std::vector<std::string>& initial_lines = {});

    void Begin() override;
    bool Available() const noexcept override;
    char Read() override;
    std::string ReadLine(size_t max_length = 256, unsigned long timeout_ms = 0, bool echo = true) override;
    util::Optional<std::string> TryReadLine(size_t max_length = 256, bool echo = true) override;
    void ClearLineBuffer() override;
    void Write(const std::string& data) override;
    void Printf(const char* format, ...) override;
    void Flush() noexcept override;

private:
    std::queue<std::string> input_queue_;
};

}  // namespace debug
}  // namespace hal
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_DEBUG_CONSOLE_HPP
