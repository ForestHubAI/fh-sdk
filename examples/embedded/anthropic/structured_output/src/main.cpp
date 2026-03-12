// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

// =============================================================================
// Anthropic Embedded Structured Output Example
// =============================================================================
// Demonstrates structured output using the Agent framework on Arduino (ESP32,
// Portenta H7). The LLM is forced to return JSON conforming to a schema (city
// information). The example parses the response and prints individual fields.
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
#include "foresthub/core/types.hpp"
#include "foresthub/platform/platform.hpp"
#include "foresthub/util/json.hpp"

using json = nlohmann::json;

// -----------------------------------------------------------------------------
// Arduino Entry Points
// -----------------------------------------------------------------------------

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
    platform->console->Printf("=== Anthropic Embedded Structured Output ===\n\n");
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
    http_cfg.host = "api.anthropic.com";
    auto http_client = platform->CreateHttpClient(http_cfg);

    // 6. Configure Anthropic provider
    foresthub::config::ClientConfig cfg;
    foresthub::config::ProviderConfig anthropic_cfg;
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

    // 8. Define JSON schema for structured output
    json city_schema = json::parse(R"({
        "type": "object",
        "properties": {
            "name":       {"type": "string", "description": "Name of the city"},
            "country":    {"type": "string", "description": "Country the city is in"},
            "population": {"type": "integer", "description": "Approximate population"}
        },
        "required": ["name", "country", "population"],
        "additionalProperties": false
    })",
                                   nullptr, false);

    foresthub::core::ResponseFormat format;
    format.name = "city_info";
    format.schema = city_schema;
    format.description = "Structured information about a city.";

    // 9. Setup agent with ResponseFormat
    auto agent = std::make_shared<foresthub::agent::Agent>("GeoBot");
    agent->WithInstructions("You are a geography expert. Answer with accurate data about the requested city.")
        .WithResponseFormat(format);

    // 10. Setup runner
    std::string model_name = "claude-haiku-4-5";
    if (!client->SupportsModel(model_name)) {
        platform->console->Printf("[ERROR] Model '%s' not supported.\n", model_name.c_str());
        platform->console->Flush();
        while (true) {
            platform->time->Delay(1000);
        }
    }

    auto runner = std::make_shared<foresthub::agent::Runner>(client, model_name);

    // 11. Execute
    std::string prompt = "Tell me about Paris.";
    platform->console->Printf("[USER] %s\n", prompt.c_str());
    platform->console->Printf("[INFO] Running agent with structured output...\n");

    auto input = std::make_shared<foresthub::core::InputString>(prompt);
    foresthub::agent::RunResultOrError result = runner->Run(agent, input);

    if (result.HasError()) {
        platform->console->Printf("[ERROR] Agent failed: %s\n", result.error.c_str());
        platform->console->Flush();
        return;
    }

    // 12. Parse structured output
    const json& output = result.result->final_output;
    platform->console->Printf("\n[RAW] %s\n\n", output.dump().c_str());

    json parsed = output.is_string() ? json::parse(output.get<std::string>(), nullptr, false) : output;

    if (parsed.is_discarded() || !parsed.is_object()) {
        platform->console->Printf("[ERROR] Failed to parse structured output.\n");
        platform->console->Flush();
        return;
    }

    platform->console->Printf("--- Parsed Fields ---\n");
    platform->console->Printf("  Name:       %s\n", parsed.value("name", "?").c_str());
    platform->console->Printf("  Country:    %s\n", parsed.value("country", "?").c_str());
    platform->console->Printf("  Population: %d\n", parsed.value("population", 0));
    platform->console->Printf("---------------------\n");
    platform->console->Printf("[STATS] Turns used: %d\n", result.result->turns);

    platform->console->Printf("\n[DONE]\n");
    platform->console->Flush();
}

void loop() {
    // One-shot example — nothing to do in loop
    platform->time->Delay(10000);
}
