#ifndef FORESTHUB_CORE_OPTIONS_HPP
#define FORESTHUB_CORE_OPTIONS_HPP

#include "foresthub/util/optional.hpp"

namespace foresthub {
namespace core {

/// Model-specific generation options.
struct Options {
    foresthub::Optional<int> max_tokens;           ///< Maximum tokens to generate.
    foresthub::Optional<float> temperature;        ///< Randomness (0.0 to 1.0).
    foresthub::Optional<int> top_k;                ///< Maximum tokens to consider.
    foresthub::Optional<float> top_p;              ///< Nucleus sampling cutoff.
    foresthub::Optional<float> frequency_penalty;  ///< Reduces repetition.
    foresthub::Optional<float> presence_penalty;   ///< Encourages new concepts.
    foresthub::Optional<int> seed;                 ///< Seed for deterministic generation.

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

}  // namespace core
}  // namespace foresthub

#endif  // FORESTHUB_CORE_OPTIONS_HPP
