# RAG (Retrieval-Augmented Generation)

RAG enriches LLM prompts with relevant document content retrieved from a knowledge base. The SDK provides a retriever interface, a remote implementation that queries the ForestHub backend, and a context formatter that produces XML for prompt injection.

## Overview

The RAG workflow has four steps:

1. **Retrieve** -- query a document collection for chunks similar to the user's question.
2. **Format** -- convert the retrieved chunks into an XML context block.
3. **Inject** -- prepend the context to the system prompt.
4. **Chat** -- send the enriched request to the LLM.

```
User question
     |
     v
Retriever.Query()  -->  ForestHub backend  -->  QueryResponse
     |
     v
FormatContext()    -->  XML context string
     |
     v
ChatRequest.WithSystemPrompt(instructions + context)
     |
     v
Client.Chat()     -->  LLM response grounded in retrieved documents
```

## Architecture

### Retriever Interface

The abstract `Retriever` class defines a single method:

```cpp
class Retriever {
public:
    virtual std::shared_ptr<QueryResponse> Query(const QueryRequest& req) = 0;
};
```

Returns `nullptr` on error.

### RemoteRetriever

The built-in implementation sends HTTP POST requests to `/rag/query` on the ForestHub backend. It reuses the same `ProviderConfig` and `HttpClient` as the ForestHub LLM provider:

```cpp
#include "foresthub/rag/remote/retriever.hpp"

foresthub::config::ProviderConfig fh_cfg;
fh_cfg.api_key = std::getenv("FORESTHUB_API_KEY");
fh_cfg.base_url = "https://fh-backend-368736749905.europe-west1.run.app";

foresthub::rag::remote::RemoteRetriever retriever(fh_cfg, http_client);
```

Authentication uses the `Device-Key` header with the same API key as the LLM provider. The retriever includes built-in retry logic for transient HTTP errors.

## Types

### QueryRequest

```cpp
foresthub::rag::QueryRequest req;
req.collection_id = "my-collection";  // ID of the document collection
req.query = "What is ForestHub?";     // Search query text
req.top_k = 5;                        // Number of results to return (default: 5)
```

### QueryResult

Each result contains a single document chunk:

| Field | Type | Description |
|-------|------|-------------|
| `chunk_id` | `std::string` | Unique identifier of the chunk |
| `document_id` | `std::string` | ID of the source document |
| `content` | `std::string` | Text content of the chunk |
| `score` | `double` | Similarity score (higher = more relevant) |

### QueryResponse

```cpp
struct QueryResponse {
    std::vector<QueryResult> results;  // Chunks ordered by relevance
};
```

## FormatContext

`FormatContext()` converts query results into an XML block suitable for prompt injection:

```cpp
#include "foresthub/rag/types.hpp"

std::string context = foresthub::rag::FormatContext(rag_response->results);
```

Output format:

```xml
<context>
<source id="1">First chunk content</source>
<source id="2">Second chunk content</source>
</context>
```

XML is used because LLMs reliably parse XML tags as structural boundaries, making it effective for separating retrieved context from user instructions.

Returns an empty string if the results vector is empty.

## RAG + Chat Workflow

The following example shows the complete retrieve-format-inject-chat pipeline:

```cpp
#include "foresthub/client.hpp"
#include "foresthub/config/config.hpp"
#include "foresthub/core/input.hpp"
#include "foresthub/core/types.hpp"
#include "foresthub/rag/remote/retriever.hpp"
#include "foresthub/rag/types.hpp"

// 1. Create retriever and client (share the same config and HTTP client)
foresthub::rag::remote::RemoteRetriever retriever(fh_cfg, http_client);
std::unique_ptr<foresthub::Client> client = foresthub::Client::Create(client_cfg, http_client);

// 2. Retrieve relevant chunks
foresthub::rag::QueryRequest rag_req;
rag_req.collection_id = "my-collection";
rag_req.query = "What is ForestHub?";
rag_req.top_k = 3;

std::shared_ptr<foresthub::rag::QueryResponse> rag_resp = retriever.Query(rag_req);
if (!rag_resp) {
    printf("RAG query failed.\n");
    return 1;
}

// 3. Format context and build system prompt
std::string context = foresthub::rag::FormatContext(rag_resp->results);
std::string system_prompt =
    "Answer the question based on the provided context.\n"
    "If the context does not contain an answer, say so honestly.\n\n" + context;

// 4. Send chat request with RAG context
foresthub::core::ChatRequest req;
req.model = "gpt-4.1-mini";
req.input = std::make_shared<foresthub::core::InputString>("What is ForestHub?");
req.WithSystemPrompt(system_prompt);

std::shared_ptr<foresthub::core::ChatResponse> resp = client->Chat(req);
if (resp) {
    printf("Response: %s\n", resp->text.c_str());
}
```

> See [`examples/pc/foresthub/rag.cpp`](../examples/pc/foresthub/rag.cpp) for the full example with platform setup and error handling.

## Further Reading

- [Getting Started](getting-started.md) -- build the SDK and send your first request
- [Provider Guide](providers.md) -- configure ForestHub and other providers
- [Agent Framework](agents.md) -- combine RAG with agent workflows
