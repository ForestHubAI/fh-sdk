// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_UTIL_STRPRINTF_HPP
#define FORESTHUB_UTIL_STRPRINTF_HPP

/// @file
/// Printf-style string formatting with single heap allocation.

#include <cstdarg>
#include <cstdio>
#include <string>

namespace foresthub {
namespace util {

/// Builds a std::string using printf-style formatting.
///
/// Uses a two-pass vsnprintf approach: first measures the exact output size,
/// then allocates once and writes. This avoids heap fragmentation from
/// repeated std::string concatenation on memory-constrained embedded devices.
///
/// @warning The format string must not contain untrusted user input.
///          Pass user data as arguments, never as the format string.
///
/// @param fmt  printf-style format string
/// @param ...  arguments matching the format specifiers
/// @return     formatted std::string (empty on encoding error)
#if defined(__GNUC__) || defined(__clang__)
// GCC/Clang: Enable compile-time format string checking.
// Argument indices: 1 = format string, 2 = first vararg (free function, no implicit 'this')
inline std::string StrPrintf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
#else
inline std::string StrPrintf(const char* fmt, ...);
#endif

inline std::string StrPrintf(const char* fmt, ...) {
    va_list args1, args2;
    va_start(args1, fmt);
    va_copy(args2, args1);

    int len = vsnprintf(nullptr, 0, fmt, args1);  // Pass 1: measure exact size
    va_end(args1);

    if (len <= 0) {
        va_end(args2);
        return "";
    }

    std::string result(static_cast<size_t>(len), '\0');               // ONE allocation, exact size
    vsnprintf(&result[0], static_cast<size_t>(len) + 1, fmt, args2);  // Pass 2: write
    va_end(args2);

    return result;  // RVO - no copy
}

}  // namespace util
}  // namespace foresthub

#endif  // FORESTHUB_UTIL_STRPRINTF_HPP
