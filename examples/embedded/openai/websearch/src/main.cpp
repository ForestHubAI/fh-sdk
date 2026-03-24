// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

// =============================================================================
// OpenAI Embedded WebSearch Example
// =============================================================================
// Demonstrates the built-in WebSearch tool on Arduino (ESP32, Portenta H7).
// Asks the same question twice: once without WebSearch (LLM uses only training
// data) and once with WebSearch (LLM searches the web for current information).
//
// Before compiling, edit env.cpp with your WiFi credentials and API key.
// =============================================================================

#include <Arduino.h>

#include "env.hpp"
#include "foresthub/agent/agent.hpp"
#include "foresthub/agent/runner.hpp"
#include "foresthub/client.hpp"
#include "foresthub/config/config.hpp"
#include "foresthub/core/input.hpp"
#include "foresthub/core/tools.hpp"
#include "foresthub/platform/platform.hpp"
#include "platform/arduino/platform.hpp"
#include "foresthub/util/json.hpp"

using json = nlohmann::json;

// Helper to run an agent and print the result.
static void RunAndPrint(std::shared_ptr<foresthub::platform::Platform>& platform,
                        std::shared_ptr<foresthub::agent::Runner>& runner,
                        std::shared_ptr<foresthub::agent::Agent>& agent, const std::string& prompt) {
    platform->console->Printf("\n[USER] %s\n", prompt.c_str());

    auto input = std::make_shared<foresthub::core::InputString>(prompt);
    platform->console->Printf("[INFO] Running agent...\n");
    foresthub::agent::RunResultOrError result = runner->Run(agent, input);

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

static std::shared_ptr<foresthub::platform::Platform> platform;

void setup() {
    // 1. Create platform context (WiFi, Serial, NTP, TLS)
    foresthub::platform::arduino::ArduinoConfig config;
    config.network.ssid = kWifiSsid;
    config.network.password = kWifiPassword;
    platform = std::make_shared<foresthub::platform::arduino::ArduinoPlatform>(config);
    if (!platform) {
        while (true) {
        }
    }

    // 2. Initialize console
    platform->console->Begin();
    platform->console->Printf("=== OpenAI Embedded WebSearch ===\n\n");
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
    foresthub::core::HttpClientConfig http_cfg;
    http_cfg.host = "api.openai.com";
    auto http_client = platform->CreateHttpClient(http_cfg);

    // 6. Configure OpenAI provider
    foresthub::config::ClientConfig cfg;
    foresthub::config::ProviderConfig oai_cfg;
    oai_cfg.api_key = kOpenaiApiKey;
    oai_cfg.supported_models = {"gpt-4.1", "gpt-4.1-mini", "gpt-4o", "gpt-4o-mini"};
    cfg.remote.openai = oai_cfg;

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
    std::string model_name = "gpt-4o-mini";
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

    auto agent_no_search = std::make_shared<foresthub::agent::Agent>("NewsBot");

    auto runner = std::make_shared<foresthub::agent::Runner>(client, model_name);

    RunAndPrint(platform, runner, agent_no_search, prompt);

    // 10. Run 2: With WebSearch
    platform->console->Printf("\n--- Run 2: WITH WebSearch ---\n");

    auto web_search = std::make_shared<foresthub::core::WebSearch>();

    auto agent_with_search = std::make_shared<foresthub::agent::Agent>("NewsBot");
    agent_with_search->AddTool(web_search);

    auto runner2 = std::make_shared<foresthub::agent::Runner>(client, model_name);
    runner2->WithMaxTurns(3);

    RunAndPrint(platform, runner2, agent_with_search, prompt);

    platform->console->Printf("\n[DONE]\n");
    platform->console->Flush();
}

void loop() {
    // One-shot example — nothing to do in loop
    platform->time->Delay(10000);
}
