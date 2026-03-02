#ifndef FORESTHUB_CONFIG_HPP
#define FORESTHUB_CONFIG_HPP

#include <string>
#include <vector>

#include "foresthub/util/optional.hpp"

namespace foresthub {
namespace config {

/// Configuration for a remote LLM provider (e.g., ForestHub backend).
struct RemoteConfig {
    std::string base_url;                       ///< Backend API base URL.
    std::string api_key;                        ///< Authentication token.
    std::vector<std::string> supported_models;  ///< Models available through this provider.
};

/// Configuration for a local LLM execution engine.
struct LocalConfig {
    std::string model_path;   ///< Path to the model weights file.
    std::string model_id;     ///< Unique identifier for routing.
    int context_size = 2048;  ///< Context window size in tokens.
    int n_threads = 4;        ///< CPU threads for inference.
    bool use_gpu = false;     ///< Enable GPU acceleration.
};

/// Top-level client configuration controlling which providers are created.
struct ClientConfig {
    foresthub::Optional<RemoteConfig> remote;  ///< Optional remote provider configuration.
    std::vector<LocalConfig> local;            ///< Zero or more local model configurations.
};

}  // namespace config
}  // namespace foresthub

#endif  // FORESTHUB_CONFIG_HPP
