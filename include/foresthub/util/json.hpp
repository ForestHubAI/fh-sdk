// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_UTIL_JSON_HPP
#define FORESTHUB_UTIL_JSON_HPP

/// @file
/// Wrapper for nlohmann/json that works around `abs` macro conflicts.
///
/// Some platform toolchains define `abs` as a C-style preprocessor macro
/// (`#define abs(x) ...`) instead of using `std::abs`. This breaks GCC's
/// `<valarray>` header, which nlohmann/json includes. We temporarily save
/// and undefine the macro around the include, then restore it so that
/// downstream code relying on the macro remains unaffected.

#ifdef abs
#pragma push_macro("abs")
#undef abs
#define FORESTHUB_RESTORE_ABS_
#endif

#include <nlohmann/json.hpp>  // IWYU pragma: export

#ifdef FORESTHUB_RESTORE_ABS_
#pragma pop_macro("abs")
#undef FORESTHUB_RESTORE_ABS_
#endif

#endif  // FORESTHUB_UTIL_JSON_HPP
