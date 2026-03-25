// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_LLM_OPTIONS_HPP
#define FORESTHUB_LLM_OPTIONS_HPP

/// @file
/// Model-specific generation options (temperature, max_tokens, etc.).

#include "foresthub/util/optional.hpp"

namespace foresthub {
namespace llm {

/// Model-specific generation options.
struct Options {
    util::Optional<int> max_tokens;           ///< Maximum tokens to generate.
    util::Optional<float> temperature;        ///< Randomness (0.0 to 1.0).
    util::Optional<int> top_k;                ///< Maximum tokens to consider.
    util::Optional<float> top_p;              ///< Nucleus sampling cutoff.
    util::Optional<float> frequency_penalty;  ///< Reduces repetition.
    util::Optional<float> presence_penalty;   ///< Encourages new concepts.
    util::Optional<int> seed;                 ///< Seed for deterministic generation.

    /// Set the maximum number of tokens to generate.
    Options& WithMaxTokens(int max_tokens) {
        this->max_tokens = max_tokens;
        return *this;
    }

    /// Set the sampling temperature.
    Options& WithTemperature(float temperature) {
        this->temperature = temperature;
        return *this;
    }

    /// Set the top-k sampling limit.
    Options& WithTopK(int top_k) {
        this->top_k = top_k;
        return *this;
    }

    /// Set the nucleus sampling cutoff.
    Options& WithTopP(float top_p) {
        this->top_p = top_p;
        return *this;
    }

    /// Set the frequency penalty.
    Options& WithFrequencyPenalty(float frequency_penalty) {
        this->frequency_penalty = frequency_penalty;
        return *this;
    }

    /// Set the presence penalty.
    Options& WithPresencePenalty(float presence_penalty) {
        this->presence_penalty = presence_penalty;
        return *this;
    }

    /// Set the random seed for deterministic output.
    Options& WithSeed(int seed) {
        this->seed = seed;
        return *this;
    }
};

}  // namespace llm
}  // namespace foresthub

#endif  // FORESTHUB_LLM_OPTIONS_HPP
