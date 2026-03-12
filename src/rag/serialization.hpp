// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_RAG_SERIALIZATION_HPP
#define FORESTHUB_RAG_SERIALIZATION_HPP

#include "foresthub/rag/types.hpp"
#include "foresthub/util/json.hpp"

namespace foresthub {
namespace rag {

using json = nlohmann::json;

/// Serialize QueryRequest to JSON (wire format: collectionId, query, topK).
void to_json(json& j, const QueryRequest& req);
/// Deserialize QueryResult from JSON (wire format: chunkId, documentId, content, score).
void from_json(const json& j, QueryResult& r);

}  // namespace rag
}  // namespace foresthub

#endif  // FORESTHUB_RAG_SERIALIZATION_HPP
