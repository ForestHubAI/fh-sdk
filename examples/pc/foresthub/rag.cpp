// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

// =============================================================================
// ForestHub PC RAG Example
// =============================================================================
// Demonstrates the full RAG + Chat workflow: query a document collection for
// relevant chunks, format them as context, and send a chat request with the
// context as system prompt. Requires FORESTHUB_API_KEY environment variable.
// Optionally set FORESTHUB_RAG_COLLECTION for the collection ID.
// =============================================================================

#include <cstdlib>
#include <string>

// Public Library Headers
#include "foresthub/llm/client.hpp"
#include "foresthub/llm/config.hpp"
#include "foresthub/llm/input.hpp"
#include "foresthub/llm/types.hpp"
#include "llm/rag/remote/retriever.hpp"
#include "foresthub/llm/rag/types.hpp"

// Application Shared Helper
#include "../platform_setup.hpp"

int main() {  // NOLINT(bugprone-exception-escape)
    // 1. Initialize Platform via HAL Factory
    auto platform = app::SetupPlatform();

    platform->console->Printf("=== ForestHub RAG + Chat (PC Platform) ===\n\n");
    platform->console->Flush();

    // 2. Load API Key from Environment
    const char* api_key_env = std::getenv("FORESTHUB_API_KEY");
    if (!api_key_env) {
        platform->console->Printf("[ERROR] Environment variable 'FORESTHUB_API_KEY' is missing.\n");
        platform->console->Flush();
        return 1;
    }
    std::string api_key = api_key_env;

    // 3. Load Collection ID from Environment
    const char* collection_env = std::getenv("FORESTHUB_RAG_COLLECTION");
    if (!collection_env) {
        platform->console->Printf("[ERROR] Environment variable 'FORESTHUB_RAG_COLLECTION' is missing.\n");
        platform->console->Flush();
        return 1;
    }

    // 4. Create HTTP Client via HAL
    foresthub::hal::HttpClientConfig http_cfg;
    http_cfg.host = "fh-backend-368736749905.europe-west1.run.app";
    auto http_client = platform->CreateHttpClient(http_cfg);

    // 5. Configure Provider
    foresthub::llm::ProviderConfig fh_cfg;
    fh_cfg.base_url = "https://fh-backend-368736749905.europe-west1.run.app";
    fh_cfg.api_key = api_key;
    fh_cfg.supported_models = {"gpt-4.1", "gpt-4.1-mini", "gpt-4o", "gpt-4o-mini"};

    // 6. Create Retriever and Client (share the same HTTP client)
    foresthub::llm::rag::remote::RemoteRetriever retriever(fh_cfg, http_client);

    foresthub::llm::ClientConfig client_cfg;
    client_cfg.remote.foresthub = fh_cfg;
    std::unique_ptr<foresthub::Client> client = foresthub::Client::Create(client_cfg, http_client);

    // 7. RAG Query
    std::string question = "Was ist ForestHub?";

    platform->console->Printf("[INFO] Querying collection '%s'...\n", collection_env);
    platform->console->Printf("       Query: %s\n", question.c_str());

    foresthub::llm::rag::QueryRequest rag_req;
    rag_req.collection_id = collection_env;
    rag_req.query = question;
    rag_req.top_k = 3;

    std::shared_ptr<foresthub::llm::rag::QueryResponse> rag_resp = retriever.Query(rag_req);
    if (!rag_resp) {
        platform->console->Printf("[ERROR] RAG query failed.\n");
        platform->console->Flush();
        return 1;
    }

    platform->console->Printf("[OK] Retrieved %d chunks\n\n", static_cast<int>(rag_resp->results.size()));

    // 8. Format Context and Build Chat Request
    std::string context = foresthub::llm::rag::FormatContext(rag_resp->results);

    std::string system_prompt =
        "Beantworte die Frage basierend auf dem bereitgestellten Kontext.\n"
        "Wenn der Kontext keine Antwort enthaelt, sage das ehrlich.\n\n" +
        context;

    auto input = std::make_shared<foresthub::llm::InputString>(question);

    foresthub::llm::ChatRequest chat_req;
    chat_req.model = "gpt-4.1-mini";
    chat_req.input = input;
    chat_req.WithSystemPrompt(system_prompt);

    // 9. Send Chat Request
    platform->console->Printf("[INFO] Sending chat request with RAG context...\n");
    std::shared_ptr<foresthub::llm::ChatResponse> chat_resp = client->Chat(chat_req);

    // 10. Output Result
    if (chat_resp) {
        platform->console->Printf("\n--------------------------------------------------\n");
        platform->console->Printf("LLM Response:\n");
        platform->console->Printf("%s\n", chat_resp->text.c_str());
        platform->console->Printf("--------------------------------------------------\n");
        platform->console->Printf("[INFO] Tokens used: %d\n", chat_resp->tokens_used);
    } else {
        platform->console->Printf("[ERROR] Chat request failed.\n");
        platform->console->Flush();
        return 1;
    }

    return 0;
}
