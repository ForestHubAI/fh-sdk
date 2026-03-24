// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PLATFORM_CONFIG_HPP
#define FORESTHUB_PLATFORM_CONFIG_HPP

/// @file platform_config.hpp
/// Compile-time validation of platform selection macros.
///
/// Exactly one platform must be defined:
///   - FORESTHUB_PLATFORM_PC        (CMake, desktop builds)
///   - FORESTHUB_PLATFORM_PC_DEBUG  (CMake, server-side debug builds)
///   - FORESTHUB_PLATFORM_ARDUINO   (PlatformIO, embedded builds)

// ---------------------------------------------------------------------------
// Exactly one platform must be defined — they are mutually exclusive.
// ---------------------------------------------------------------------------
#if defined(FORESTHUB_PLATFORM_PC) + defined(FORESTHUB_PLATFORM_PC_DEBUG) + defined(FORESTHUB_PLATFORM_ARDUINO) != 1
#error "Exactly one FORESTHUB_PLATFORM_* must be defined (PC, PC_DEBUG, or ARDUINO)"
#endif

#endif  // FORESTHUB_PLATFORM_CONFIG_HPP
