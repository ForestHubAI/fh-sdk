// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_UTIL_OPTIONAL_HPP
#define FORESTHUB_UTIL_OPTIONAL_HPP

/// @file
/// Minimal Optional<T> polyfill for C++14 compatibility.

#include <utility>

namespace foresthub {
namespace util {

/// Minimal Optional<T> polyfill for C++14 compatibility.
///
/// Used for non-string optional fields (int, float, structs) where a
/// default/zero value would be ambiguous. String-type optional fields
/// should use plain std::string (empty = not set).
template <typename T>
struct Optional {
    bool has_value = false;  ///< Whether a value is stored.
    T value{};               ///< Stored value (default-constructed when empty).

    /// Default-construct without a stored value.
    Optional() = default;

    /// Construct with a copy of the given value.
    Optional(const T& val) : has_value(true), value(val) {}

    /// Construct by moving the given value.
    Optional(T&& val) : has_value(true), value(std::move(val)) {}

    /// Returns true if a value is stored.
    bool HasValue() const { return has_value; }

    /// Contextual conversion to bool (true if a value is stored).
    explicit operator bool() const { return has_value; }

    /// Returns a reference to the stored value (undefined if empty).
    const T& operator*() const { return value; }
    /// Returns a mutable reference to the stored value (undefined if empty).
    T& operator*() { return value; }
    /// Returns a pointer to the stored value (undefined if empty).
    const T* operator->() const { return &value; }
    /// Returns a mutable pointer to the stored value (undefined if empty).
    T* operator->() { return &value; }

    /// Assign a value (sets has_value to true).
    Optional& operator=(const T& val) {
        has_value = true;
        value = val;
        return *this;
    }

    /// Move-assign a value (sets has_value to true).
    Optional& operator=(T&& val) {
        has_value = true;
        value = std::move(val);
        return *this;
    }
};

}  // namespace util
}  // namespace foresthub

#endif  // FORESTHUB_UTIL_OPTIONAL_HPP
