// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

// =============================================================================
// Anthropic PC Structured Output Example
// =============================================================================
// Demonstrates structured output using the Agent framework. The LLM is forced
// to return JSON conforming to a schema (city information). The example then
// parses the structured response and prints individual fields.
// Requires ANTHROPIC_API_KEY environment variable.
// =============================================================================

#include <cstdlib>
#include <string>

#include "foresthub/util/json.hpp"

// Core & Agent Headers
#include "foresthub/llm/agent/agent.hpp"
#include "foresthub/llm/agent/runner.hpp"
#include "foresthub/llm/client.hpp"
#include "foresthub/llm/config.hpp"
#include "foresthub/llm/input.hpp"
#include "foresthub/llm/types.hpp"

// Application Shared Helper
#include "../platform_setup.hpp"

using json = nlohmann::json;

int main() {  // NOLINT(bugprone-exception-escape)
    // 1. Initialize Platform via HAL Factory
    auto platform = app::SetupPlatform();

    platform->console->Printf("=== Anthropic Structured Output (PC Platform) ===\n\n");
    platform->console->Flush();

    // --- Setup Client ---
    const char* api_key_env = std::getenv("ANTHROPIC_API_KEY");
    if (!api_key_env) {
        platform->console->Printf("[ERROR] Environment variable 'ANTHROPIC_API_KEY' is missing.\n");
        platform->console->Flush();
        return 1;
    }

    foresthub::hal::HttpClientConfig http_cfg;
    http_cfg.host = "api.anthropic.com";
    auto http_client = platform->CreateHttpClient(http_cfg);

    foresthub::llm::ClientConfig cfg;
    foresthub::llm::ProviderConfig anthropic_cfg;
    std::string api_key = api_key_env;
    anthropic_cfg.api_key = api_key;
    anthropic_cfg.supported_models = {"claude-sonnet-4-6", "claude-haiku-4-5", "claude-opus-4-6"};
    cfg.remote.anthropic = anthropic_cfg;

    std::shared_ptr<foresthub::Client> client = foresthub::Client::Create(cfg, http_client);

    // --- Define JSON Schema for Structured Output ---
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

    foresthub::llm::ResponseFormat format;
    format.name = "city_info";
    format.schema = city_schema;
    format.description = "Structured information about a city.";

    // --- Setup Agent with ResponseFormat ---
    auto agent = std::make_shared<foresthub::llm::agent::Agent>("GeoBot");
    agent->WithInstructions("You are a geography expert. Answer with accurate data about the requested city.")
        .WithResponseFormat(format);

    // --- Setup Runner ---
    std::string model_name = "claude-haiku-4-5";
    if (!client->SupportsModel(model_name)) {
        platform->console->Printf("[ERROR] Model '%s' is not supported by any configured provider.\n",
                                  model_name.c_str());
        platform->console->Flush();
        return 1;
    }

    auto runner = std::make_shared<foresthub::llm::agent::Runner>(client, model_name);

    // --- Execute ---
    std::string prompt = "Tell me about Paris.";
    platform->console->Printf("[USER] %s\n", prompt.c_str());
    platform->console->Printf("[INFO] Running agent with structured output...\n");

    auto input = std::make_shared<foresthub::llm::InputString>(prompt);
    foresthub::llm::agent::RunResultOrError result = runner->Run(agent, input);

    if (result.HasError()) {
        platform->console->Printf("[ERROR] Agent failed: %s\n", result.error.c_str());
        platform->console->Flush();
        return 1;
    }

    // --- Parse Structured Output ---
    const json& output = result.result->final_output;
    platform->console->Printf("\n[RAW] %s\n\n", output.dump().c_str());

    // final_output may be a JSON string (needs parsing) or already an object
    json parsed = output.is_string() ? json::parse(output.get<std::string>(), nullptr, false) : output;

    if (parsed.is_discarded() || !parsed.is_object()) {
        platform->console->Printf("[ERROR] Failed to parse structured output.\n");
        platform->console->Flush();
        return 1;
    }

    platform->console->Printf("--- Parsed Fields ---\n");
    platform->console->Printf("  Name:       %s\n", parsed.value("name", "?").c_str());
    platform->console->Printf("  Country:    %s\n", parsed.value("country", "?").c_str());
    platform->console->Printf("  Population: %d\n", parsed.value("population", 0));
    platform->console->Printf("---------------------\n");
    platform->console->Printf("[STATS] Turns used: %d\n", result.result->turns);

    platform->console->Printf("\n[DONE]\n");
    platform->console->Flush();

    return 0;
}
