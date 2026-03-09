#ifndef FORESTHUB_RAG_RETRIEVER_HPP
#define FORESTHUB_RAG_RETRIEVER_HPP

#include <memory>

#include "foresthub/rag/types.hpp"

namespace foresthub {
namespace rag {

/// Abstract interface for document retrieval.
///
/// Implementations provide different backends (remote API, local vector DB).
class Retriever {
public:
    virtual ~Retriever() = default;

    /// Query for similar document chunks.
    /// @param req Query parameters (collection, query text, result count).
    /// @return Query response with matching chunks, or nullptr on error.
    virtual std::shared_ptr<QueryResponse> Query(const QueryRequest& req) = 0;
};

}  // namespace rag
}  // namespace foresthub

#endif  // FORESTHUB_RAG_RETRIEVER_HPP
