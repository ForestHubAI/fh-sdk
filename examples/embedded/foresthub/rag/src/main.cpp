// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

// =============================================================================
// ForestHub Embedded RAG Example
// =============================================================================
// RAG + Chat workflow on Arduino (ESP32, Portenta H7).
// Queries a document collection, formats the results as context, and sends
// a chat request with the context as system prompt.
//
// Before compiling, edit env.cpp with your WiFi credentials, API key,
// and collection ID.
// =============================================================================

#include <Arduino.h>

#include "env.hpp"
#include "foresthub/client.hpp"
#include "foresthub/config/config.hpp"
#include "foresthub/core/input.hpp"
#include "foresthub/core/types.hpp"
#include "foresthub/platform/platform.hpp"
#include "foresthub/rag/remote/retriever.hpp"
#include "foresthub/rag/types.hpp"

static std::shared_ptr<foresthub::platform::PlatformContext> platform;

void setup() {
    // 1. Create platform context (WiFi, Serial, NTP, TLS)
    foresthub::platform::PlatformConfig config;
    config.network.ssid = kWifiSsid;
    config.network.password = kWifiPassword;
    platform = foresthub::platform::CreatePlatform(config);
    if (!platform) {
        while (true) {
        }
    }

    // 2. Initialize console
    platform->console->Begin();
    platform->console->Printf("=== ForestHub Embedded RAG ===\n\n");
    platform->console->Flush();

    // 3. Connect network (retry up to 3 times)
    std::string net_err;
    for (int attempt = 1; attempt <= 3; ++attempt) {
        net_err = platform->network->Connect();
        if (net_err.empty()) break;
        platform->console->Printf("[WARN] Network attempt %d/3: %s\n", attempt, net_err.c_str());
        if (attempt < 3) {
            platform->console->Printf("[INFO] Retrying in 2s...\n");
            platform->console->Flush();
            platform->time->Delay(2000);
        }
    }
    if (!net_err.empty()) {
        platform->console->Printf("[FATAL] Network: %s\n", net_err.c_str());
        platform->console->Flush();
        while (true) {
            platform->time->Delay(1000);
        }
    }
    platform->console->Printf("[OK] Network connected (%s)\n", platform->network->GetLocalIp().c_str());

    // 4. Synchronize time
    std::string time_err = platform->time->SyncTime();
    if (!time_err.empty()) {
        platform->console->Printf("[FATAL] Time sync: %s\n", time_err.c_str());
        platform->console->Flush();
        while (true) {
            platform->time->Delay(1000);
        }
    }
    platform->console->Printf("[OK] Time synced\n\n");

    // 5. Create HTTP client via HAL
    foresthub::platform::HttpClientConfig http_cfg;
    http_cfg.host = "fh-backend-368736749905.europe-west1.run.app";
    auto http_client = platform->CreateHttpClient(http_cfg);

    // 6. Configure provider
    foresthub::config::ProviderConfig fh_cfg;
    fh_cfg.base_url = "https://fh-backend-368736749905.europe-west1.run.app";
    fh_cfg.api_key = kForesthubApiKey;
    fh_cfg.supported_models = {"gpt-4.1", "gpt-4.1-mini", "gpt-4o", "gpt-4o-mini"};

    // 7. Create Retriever and Client (share HTTP client)
    foresthub::rag::remote::RemoteRetriever retriever(fh_cfg, http_client);

    foresthub::config::ClientConfig client_cfg;
    client_cfg.remote.foresthub = fh_cfg;
    std::unique_ptr<foresthub::Client> client = foresthub::Client::Create(client_cfg, http_client);

    // 8. RAG Query
    std::string question = "Was ist ForestHub?";

    platform->console->Printf("[INFO] Querying collection '%s'...\n", kRagCollectionId);
    platform->console->Printf("       Query: %s\n", question.c_str());

    foresthub::rag::QueryRequest rag_req;
    rag_req.collection_id = kRagCollectionId;
    rag_req.query = question;
    rag_req.top_k = 3;

    std::shared_ptr<foresthub::rag::QueryResponse> rag_resp = retriever.Query(rag_req);
    if (!rag_resp) {
        platform->console->Printf("[ERROR] RAG query failed.\n");
        platform->console->Flush();
        while (true) {
            platform->time->Delay(1000);
        }
    }

    platform->console->Printf("[OK] Retrieved %d chunks\n\n", static_cast<int>(rag_resp->results.size()));

    // 9. Format context and send chat request
    std::string context = foresthub::rag::FormatContext(rag_resp->results);

    std::string system_prompt =
        "Beantworte die Frage basierend auf dem bereitgestellten Kontext.\n"
        "Wenn der Kontext keine Antwort enthaelt, sage das ehrlich.\n\n" +
        context;

    auto input = std::make_shared<foresthub::core::InputString>(question);

    foresthub::core::ChatRequest chat_req;
    chat_req.model = "gpt-4.1-mini";
    chat_req.input = input;
    chat_req.WithSystemPrompt(system_prompt);

    platform->console->Printf("[INFO] Sending chat request with RAG context...\n");
    std::shared_ptr<foresthub::core::ChatResponse> chat_resp = client->Chat(chat_req);

    // 10. Print result
    if (chat_resp) {
        platform->console->Printf("\n--------------------------------------------------\n");
        platform->console->Printf("LLM Response:\n");
        platform->console->Printf("%s\n", chat_resp->text.c_str());
        platform->console->Printf("--------------------------------------------------\n");
        platform->console->Printf("[INFO] Tokens used: %d\n", chat_resp->tokens_used);
    } else {
        platform->console->Printf("[ERROR] Chat request failed.\n");
    }

    platform->console->Printf("\n[DONE]\n");
    platform->console->Flush();
}

void loop() {
    // One-shot example — nothing to do in loop
    platform->time->Delay(10000);
}
