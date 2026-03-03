#ifndef FORESTHUB_CONFIG_HPP
#define FORESTHUB_CONFIG_HPP

#include <string>
#include <vector>

#include "foresthub/util/optional.hpp"

namespace foresthub {
namespace config {

/// Shared configuration for any remote LLM provider.
///
/// Each provider interprets the fields according to its own API:
/// - `base_url`: Empty means the provider uses its built-in default.
/// - `supported_models`: Empty means the provider accepts all models.
struct ProviderConfig {
    std::string api_key;                        ///< Authentication token.
    std::string base_url;                       ///< API base URL (empty = provider default).
    std::vector<std::string> supported_models;  ///< Models available through this provider.
};

/// Container for all remote provider configurations.
struct RemoteProviders {
    foresthub::Optional<ProviderConfig> foresthub;  ///< ForestHub backend provider.
    foresthub::Optional<ProviderConfig> openai;     ///< Direct OpenAI Responses API provider.
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
    RemoteProviders remote;          ///< Remote provider configurations.
    std::vector<LocalConfig> local;  ///< Zero or more local model configurations.
};

}  // namespace config
}  // namespace foresthub

#endif  // FORESTHUB_CONFIG_HPP
