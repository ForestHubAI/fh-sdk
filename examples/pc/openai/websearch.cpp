// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

// =============================================================================
// OpenAI PC WebSearch Example
// =============================================================================
// Demonstrates the built-in WebSearch tool. Asks the same question twice:
// once without WebSearch (LLM uses only training data) and once with WebSearch
// (LLM searches the web for current information).
// Requires OPENAI_API_KEY environment variable.
// =============================================================================

#include <cstdlib>
#include <string>

// Core & Agent Headers
#include "foresthub/llm/agent/agent.hpp"
#include "foresthub/llm/agent/runner.hpp"
#include "foresthub/llm/client.hpp"
#include "foresthub/llm/config.hpp"
#include "foresthub/llm/input.hpp"
#include "foresthub/llm/tools.hpp"

// Application Shared Helper
#include "../platform_setup.hpp"

using json = nlohmann::json;

// Helper to run an agent and print the result.
static void RunAndPrint(std::shared_ptr<foresthub::hal::Platform>& platform,
                        std::shared_ptr<foresthub::llm::agent::Runner>& runner,
                        std::shared_ptr<foresthub::llm::agent::Agent>& agent, const std::string& prompt) {
    platform->console->Printf("\n[USER] %s\n", prompt.c_str());

    auto input = std::make_shared<foresthub::llm::InputString>(prompt);
    platform->console->Printf("[INFO] Running agent...\n");
    foresthub::llm::agent::RunResultOrError result = runner->Run(agent, input);

    if (result.HasError()) {
        platform->console->Printf("[ERROR] Agent failed: %s\n", result.error.c_str());
    } else {
        const json& output = result.result->final_output;
        if (output.is_string()) {
            platform->console->Printf("[AGENT] %s\n", output.get<std::string>().c_str());
        } else {
            platform->console->Printf("[AGENT] %s\n", output.dump().c_str());
        }
        platform->console->Printf("[STATS] Turns used: %d\n", result.result->turns);
    }
}

int main() {  // NOLINT(bugprone-exception-escape)
    // 1. Initialize Platform via HAL Factory
    auto platform = app::SetupPlatform();

    platform->console->Printf("=== OpenAI WebSearch (PC Platform) ===\n\n");
    platform->console->Flush();

    // --- Setup Client ---
    const char* api_key_env = std::getenv("OPENAI_API_KEY");
    if (!api_key_env) {
        platform->console->Printf("[ERROR] Environment variable 'OPENAI_API_KEY' is missing.\n");
        platform->console->Flush();
        return 1;
    }

    foresthub::hal::HttpClientConfig http_cfg;
    http_cfg.host = "api.openai.com";
    auto http_client = platform->CreateHttpClient(http_cfg);

    foresthub::llm::ClientConfig cfg;
    foresthub::llm::ProviderConfig oai_cfg;
    std::string api_key = api_key_env;
    oai_cfg.api_key = api_key;
    oai_cfg.supported_models = {"gpt-4.1", "gpt-4.1-mini", "gpt-4o", "gpt-4o-mini"};
    cfg.remote.openai = oai_cfg;

    std::shared_ptr<foresthub::Client> client = foresthub::Client::Create(cfg, http_client);

    // --- Common setup ---
    std::string model_name = "gpt-4o-mini";
    if (!client->SupportsModel(model_name)) {
        platform->console->Printf("[ERROR] Model '%s' is not supported by any configured provider.\n",
                                  model_name.c_str());
        platform->console->Flush();
        return 1;
    }

    std::string prompt = "What are the latest news today? Give a short answer.";

    // --- Run 1: Without WebSearch ---
    platform->console->Printf("--- Run 1: WITHOUT WebSearch ---\n");

    auto agent_no_search = std::make_shared<foresthub::llm::agent::Agent>("NewsBot");

    auto runner = std::make_shared<foresthub::llm::agent::Runner>(client, model_name);

    RunAndPrint(platform, runner, agent_no_search, prompt);

    // --- Run 2: With WebSearch ---
    platform->console->Printf("\n--- Run 2: WITH WebSearch ---\n");

    auto web_search = std::make_shared<foresthub::llm::WebSearch>();

    auto agent_with_search = std::make_shared<foresthub::llm::agent::Agent>("NewsBot");
    agent_with_search->AddTool(web_search);

    auto runner2 = std::make_shared<foresthub::llm::agent::Runner>(client, model_name);
    runner2->WithMaxTurns(3);

    RunAndPrint(platform, runner2, agent_with_search, prompt);

    platform->console->Printf("\n[DONE]\n");
    platform->console->Flush();

    return 0;
}
