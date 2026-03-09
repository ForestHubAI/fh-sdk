#include "rag/serialization.hpp"

namespace foresthub {
namespace rag {

void to_json(json& j, const QueryRequest& req) {
    j = json{{"collectionId", req.collection_id}, {"query", req.query}, {"topK", req.top_k}};
}

void from_json(const json& j, QueryResult& r) {
    r.chunk_id = j.value("chunkId", "");
    r.document_id = j.value("documentId", "");
    r.content = j.value("content", "");
    r.score = j.value("score", 0.0);
}

}  // namespace rag
}  // namespace foresthub
