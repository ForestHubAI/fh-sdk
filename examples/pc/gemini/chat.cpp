// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

// =============================================================================
// Gemini PC Chat Example
// =============================================================================
// Simple chat request on PC. Sends a single prompt to the LLM and prints the
// response. Requires GEMINI_API_KEY environment variable.
// =============================================================================

#include <cstdlib>
#include <string>

// Public Library Headers
#include "foresthub/client.hpp"
#include "foresthub/config/config.hpp"
#include "foresthub/core/input.hpp"
#include "foresthub/core/types.hpp"

// Application Shared Helper
#include "../platform_setup.hpp"

int main() {  // NOLINT(bugprone-exception-escape)
    // 1. Initialize Platform via HAL Factory
    auto platform = app::SetupPlatform();

    // Print banner
    platform->console->Printf("=== Gemini Chat (PC Platform) ===\n\n");
    platform->console->Flush();

    // 2. Load API Key from Environment
    // Security Best Practice: Never hardcode API keys in source code.
    const char* api_key_env = std::getenv("GEMINI_API_KEY");
    if (!api_key_env) {
        platform->console->Printf("[ERROR] Environment variable 'GEMINI_API_KEY' is missing.\n");
        platform->console->Printf("        Please export it before running: export GEMINI_API_KEY=...\n");
        platform->console->Flush();
        return 1;
    }
    std::string api_key = api_key_env;

    // 3. Create HTTP Client via HAL
    foresthub::core::HttpClientConfig http_cfg;
    http_cfg.host = "generativelanguage.googleapis.com";
    auto http_client = platform->CreateHttpClient(http_cfg);

    // 4. Configure the Client
    foresthub::config::ClientConfig cfg;

    // Configure the Remote Provider (Gemini)
    foresthub::config::ProviderConfig gemini_cfg;
    gemini_cfg.api_key = api_key;

    // Define which models should be routed to this provider.
    // The client uses this list to decide where to send the request.
    gemini_cfg.supported_models = {"gemini-2.5-pro", "gemini-2.5-flash", "gemini-2.5-flash-lite"};

    cfg.remote.gemini = gemini_cfg;

    // 5. Create the Client Instance
    // This factory method wires everything together (Providers + HttpClient).
    std::unique_ptr<foresthub::Client> client = foresthub::Client::Create(cfg, http_client);

    // 6. Health Check (Optional)
    // Verifies if the provider is reachable before we start chatting.
    platform->console->Printf("[INFO] Checking Provider Health...\n");
    std::string health_error = client->Health();
    if (!health_error.empty()) {
        platform->console->Printf("[ERROR] Health check failed: %s\n", health_error.c_str());
        platform->console->Flush();
        return 1;
    }
    platform->console->Printf("[INFO] System is healthy.\n\n");

    // 7. Build the Chat Request
    // We want to ask a simple question to test the full pipeline.
    std::string model_name = "gemini-2.5-flash";
    std::string prompt = "Was ist die Hauptstadt von Frankreich? Antworte kurz.";

    platform->console->Printf("[INFO] Sending request...\n");
    platform->console->Printf("       Model: %s\n", model_name.c_str());
    platform->console->Printf("       Input: %s\n", prompt.c_str());
    auto input = std::make_shared<foresthub::core::InputString>(prompt);

    foresthub::core::ChatRequest req;
    req.model = model_name;
    req.input = input;
    // Optional: Add a system prompt to steer behavior
    req.WithSystemPrompt("You are a helpful geography assistant.");

    // 8. Execute Request
    // This blocks until the HTTP response returns (via cpr).
    std::shared_ptr<foresthub::core::ChatResponse> response = client->Chat(req);

    // 9. Output Result
    if (response) {
        platform->console->Printf("\n--------------------------------------------------\n");
        platform->console->Printf("LLM Response:\n");
        platform->console->Printf("%s\n", response->text.c_str());
        platform->console->Printf("--------------------------------------------------\n");
        platform->console->Printf("[INFO] Tokens used: %d\n", response->tokens_used);
        platform->console->Printf("[INFO] Request ID:  %s\n", response->response_id.c_str());
    } else {
        platform->console->Printf("[ERROR] Request failed. Verify your API Key and Internet Connection.\n");
        platform->console->Flush();
        return 1;
    }

    return 0;
}
