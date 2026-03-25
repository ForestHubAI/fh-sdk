// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_RAG_REMOTE_RETRIEVER_HPP
#define FORESTHUB_RAG_REMOTE_RETRIEVER_HPP

/// @file
/// RemoteRetriever that queries the ForestHub backend via HTTP.

#include <memory>
#include <string>

#include "foresthub/llm/config/config.hpp"
#include "foresthub/llm/http_client.hpp"
#include "foresthub/llm/rag/retriever.hpp"

namespace foresthub {
namespace rag {
namespace remote {

/// Retriever implementation that queries the ForestHub backend via HTTP.
///
/// Sends POST requests to `/rag/query` with Device-Key authentication.
/// Reuses the same HttpClient and ProviderConfig as ForestHub LLM providers.
class RemoteRetriever : public Retriever {
public:
    /// @param cfg Provider config (api_key used as Device-Key, base_url as backend URL).
    /// @param http_client Shared HTTP client for network requests.
    RemoteRetriever(const config::ProviderConfig& cfg, std::shared_ptr<llm::HttpClient> http_client);

    std::shared_ptr<QueryResponse> Query(const QueryRequest& req) override;

private:
    std::shared_ptr<llm::HttpClient> http_;
    std::string api_key_;
    std::string base_url_;
    llm::HttpClient::Headers cached_headers_;
};

}  // namespace remote
}  // namespace rag
}  // namespace foresthub

#endif  // FORESTHUB_RAG_REMOTE_RETRIEVER_HPP
