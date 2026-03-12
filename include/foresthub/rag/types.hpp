#ifndef FORESTHUB_RAG_TYPES_HPP
#define FORESTHUB_RAG_TYPES_HPP

/// @file
/// RAG query and response types.

#include <string>
#include <vector>

namespace foresthub {
namespace rag {

/// Request parameters for a RAG similarity query.
struct QueryRequest {
    std::string collection_id;  ///< ID of the collection to search.
    std::string query;          ///< Search query text.
    int top_k = 5;              ///< Number of results to return.
};

/// A single document chunk returned from a RAG query.
struct QueryResult {
    std::string chunk_id;     ///< Unique identifier of the chunk.
    std::string document_id;  ///< ID of the source document.
    std::string content;      ///< Text content of the chunk.
    double score = 0.0;       ///< Similarity score (higher = more relevant).
};

/// Response from a RAG query containing matching document chunks.
struct QueryResponse {
    std::vector<QueryResult> results;  ///< Matching chunks ordered by relevance.
};

/// Format query results as XML context block for LLM prompt injection.
///
/// Output format:
/// ```
/// <context>
/// <source id="1">chunk content</source>
/// <source id="2">chunk content</source>
/// </context>
/// ```
///
/// @param results Vector of query results to format.
/// @return XML-formatted context string, or empty string if results are empty.
inline std::string FormatContext(const std::vector<QueryResult>& results) {
    if (results.empty()) {
        return "";
    }
    std::string out = "<context>\n";
    for (size_t i = 0; i < results.size(); ++i) {
        out += "<source id=\"" + std::to_string(i + 1) + "\">" + results[i].content + "</source>\n";
    }
    out += "</context>";
    return out;
}

}  // namespace rag
}  // namespace foresthub

#endif  // FORESTHUB_RAG_TYPES_HPP
