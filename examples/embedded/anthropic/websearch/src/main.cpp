// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

// =============================================================================
// Anthropic Embedded WebSearch Example
// =============================================================================
// Demonstrates the built-in WebSearch tool on Arduino (ESP32, Portenta H7).
// Asks the same question twice: once without WebSearch (LLM uses only training
// data) and once with WebSearch (LLM searches the web for current information).
//
// Before compiling, edit env.cpp with your WiFi credentials and API key.
// =============================================================================

#include <Arduino.h>

#include "env.hpp"
#include "foresthub/llm/agent/agent.hpp"
#include "foresthub/llm/agent/runner.hpp"
#include "foresthub/llm/client.hpp"
#include "foresthub/llm/config.hpp"
#include "foresthub/llm/input.hpp"
#include "foresthub/llm/tools.hpp"
#include "foresthub/hal/platform.hpp"
#include "hal/arduino/platform.hpp"
#include "foresthub/util/json.hpp"

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

// -----------------------------------------------------------------------------
// Arduino Entry Points
// -----------------------------------------------------------------------------

static std::shared_ptr<foresthub::hal::Platform> platform;

void setup() {
    // 1. Create platform context (WiFi, Serial, NTP, TLS)
    foresthub::hal::arduino::ArduinoConfig config;
    config.network.ssid = kWifiSsid;
    config.network.password = kWifiPassword;
    platform = std::make_shared<foresthub::hal::arduino::ArduinoPlatform>(config);
    if (!platform) {
        while (true) {
        }
    }

    // 2. Initialize console
    platform->console->Begin();
    platform->console->Printf("=== Anthropic Embedded WebSearch ===\n\n");
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
    foresthub::hal::HttpClientConfig http_cfg;
    http_cfg.host = "api.anthropic.com";
    auto http_client = platform->CreateHttpClient(http_cfg);

    // 6. Configure Anthropic provider
    foresthub::llm::ClientConfig cfg;
    foresthub::llm::ProviderConfig anthropic_cfg;
    anthropic_cfg.api_key = kAnthropicApiKey;
    anthropic_cfg.supported_models = {"claude-sonnet-4-6", "claude-haiku-4-5", "claude-opus-4-6"};
    cfg.remote.anthropic = anthropic_cfg;

    std::shared_ptr<foresthub::Client> client = foresthub::Client::Create(cfg, http_client);

    // 7. Health check
    platform->console->Printf("[INFO] Checking provider health...\n");
    std::string health_err = client->Health();
    if (!health_err.empty()) {
        platform->console->Printf("[ERROR] Health check failed: %s\n", health_err.c_str());
        platform->console->Flush();
        while (true) {
            platform->time->Delay(1000);
        }
    }
    platform->console->Printf("[OK] Provider healthy\n\n");

    // 8. Common setup
    std::string model_name = "claude-haiku-4-5";
    if (!client->SupportsModel(model_name)) {
        platform->console->Printf("[ERROR] Model '%s' not supported.\n", model_name.c_str());
        platform->console->Flush();
        while (true) {
            platform->time->Delay(1000);
        }
    }

    std::string prompt = "What are the latest news today? Give a short answer.";

    // 9. Run 1: Without WebSearch
    platform->console->Printf("--- Run 1: WITHOUT WebSearch ---\n");

    auto agent_no_search = std::make_shared<foresthub::llm::agent::Agent>("NewsBot");

    auto runner = std::make_shared<foresthub::llm::agent::Runner>(client, model_name);

    RunAndPrint(platform, runner, agent_no_search, prompt);

    // 10. Run 2: With WebSearch
    platform->console->Printf("\n--- Run 2: WITH WebSearch ---\n");
    platform->console->Printf("[WARN] Anthropic does not support WebSearch — the tool will be silently filtered.\n");
    platform->console->Printf("[WARN] Both runs will produce similar results using only training data.\n");

    auto web_search = std::make_shared<foresthub::llm::WebSearch>();

    auto agent_with_search = std::make_shared<foresthub::llm::agent::Agent>("NewsBot");
    agent_with_search->AddTool(web_search);

    auto runner2 = std::make_shared<foresthub::llm::agent::Runner>(client, model_name);
    runner2->WithMaxTurns(3);

    RunAndPrint(platform, runner2, agent_with_search, prompt);

    platform->console->Printf("\n[DONE]\n");
    platform->console->Flush();
}

void loop() {
    // One-shot example — nothing to do in loop
    platform->time->Delay(10000);
}
