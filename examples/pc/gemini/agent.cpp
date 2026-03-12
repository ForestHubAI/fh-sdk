// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

// =============================================================================
// Gemini PC Agent Example
// =============================================================================
// Demonstrates the agent framework with tool calling. An agent uses a weather
// tool to answer questions. Requires GEMINI_API_KEY environment variable.
// =============================================================================

#include <cstdlib>
#include <string>

#include "foresthub/util/json.hpp"

// Core & Agent Headers
#include "foresthub/agent/agent.hpp"
#include "foresthub/agent/runner.hpp"
#include "foresthub/client.hpp"
#include "foresthub/config/config.hpp"
#include "foresthub/core/input.hpp"
#include "foresthub/core/options.hpp"
#include "foresthub/core/tools.hpp"

// Application Shared Helper
#include "../platform_setup.hpp"

using json = nlohmann::json;

// -----------------------------------------------------------------------------
// 1. Definition of the Weather Tool
// -----------------------------------------------------------------------------

// Argument struct populated by the LLM
struct WeatherArgs {
    std::string city;
};

// JSON deserialization for arguments.
// To avoid exceptions, we use .value() which returns a default if the key is missing.
void from_json(const json& j, WeatherArgs& args) {
    args.city = j.value("city", "");
}

// The actual function logic to be executed
json GetWeather(const WeatherArgs& args) {
    const std::string& city = args.city;

    // Simple logic for the test cities
    if (city == "Berlin") return json{{"city", "Berlin"}, {"temperature", "15C"}, {"condition", "cloudy"}};
    if (city == "London") return json{{"city", "London"}, {"temperature", "12C"}, {"condition", "light rain"}};
    if (city == "Madrid") return json{{"city", "Madrid"}, {"temperature", "28C"}, {"condition", "sunny"}};
    if (city == "Paris") return json{{"city", "Paris"}, {"temperature", "18C"}, {"condition", "partly sunny"}};

    return json{
        {"error", "Weather data for city '" + city + "' is not available. Available: Berlin, London, Madrid, Paris."}};
}

// -----------------------------------------------------------------------------
// 2. Main Application
// -----------------------------------------------------------------------------

int main() {  // NOLINT(bugprone-exception-escape)
    // 1. Initialize Platform via HAL Factory
    auto platform = app::SetupPlatform();

    // Print banner
    platform->console->Printf("=== Gemini Agent (PC Platform) ===\n\n");
    platform->console->Flush();

    // --- Setup Client ---
    const char* api_key_env = std::getenv("GEMINI_API_KEY");
    if (!api_key_env) {
        platform->console->Printf("[ERROR] Environment variable 'GEMINI_API_KEY' is missing.\n");
        platform->console->Flush();
        return 1;
    }

    // Create HTTP client via HAL
    foresthub::platform::HttpClientConfig http_cfg;
    http_cfg.host = "generativelanguage.googleapis.com";
    auto http_client = platform->CreateHttpClient(http_cfg);

    foresthub::config::ClientConfig cfg;
    foresthub::config::ProviderConfig gemini_cfg;
    std::string api_key = api_key_env;
    gemini_cfg.api_key = api_key;
    // Ensure selected models support tool calling
    gemini_cfg.supported_models = {"gemini-2.5-pro", "gemini-2.5-flash", "gemini-2.5-flash-lite"};
    cfg.remote.gemini = gemini_cfg;

    std::shared_ptr<foresthub::Client> client = foresthub::Client::Create(cfg, http_client);

    // --- Setup Tool ---

    // Define JSON schema for the tool so the LLM understands how to use it
    json weather_tool_schema = json::parse(
        R"({"type":"object","properties":{"city":{"type":"string","description":"The city to get the weather for (e.g. Berlin, London)."}},"required":["city"]})",
        nullptr, false);

    // Create FunctionTool via Factory
    auto weather_tool =
        foresthub::core::NewFunctionTool<WeatherArgs, json>("get_weather",                              // Tool Name
                                                            "Returns the current weather for a city.",  // Description
                                                            weather_tool_schema,  // Schema as json
                                                            GetWeather            // C++ Function
        );

    // --- Setup Agent ---

    auto agent = std::make_shared<foresthub::agent::Agent>("WeatherBot");
    agent->WithInstructions("You are a helpful assistant. If asked about weather, use the provided tool.")
        .WithOptions(foresthub::core::Options().WithTemperature(0.7f).WithMaxTokens(1024))
        .AddTool(weather_tool);

    // --- Setup Runner ---

    std::string model_name = "gemini-2.5-flash";

    // Ensure the model is supported by the configured providers
    if (!client->SupportsModel(model_name)) {
        platform->console->Printf("[ERROR] Model '%s' is not supported by any configured provider.\n",
                                  model_name.c_str());
        platform->console->Flush();
        return 1;
    }

    auto runner = std::make_shared<foresthub::agent::Runner>(client, model_name);
    runner->WithMaxTurns(5);  // Limit turns to avoid infinite loops

    // --- Interaction 1: Question about weather (Tool usage expected) ---

    std::string prompt = "How is the weather in Madrid and how does it look in Berlin?";
    platform->console->Printf("\n[USER] %s\n", prompt.c_str());

    auto input = std::make_shared<foresthub::core::InputString>(prompt);

    platform->console->Printf("[INFO] Running agent... (calling tools if necessary)\n");
    foresthub::agent::RunResultOrError result = runner->Run(agent, input);

    if (result.HasError()) {
        platform->console->Printf("[ERROR] Agent execution failed: %s\n", result.error.c_str());
        platform->console->Flush();
        return 1;
    } else {
        // final_output is json — extract string or dump as fallback
        const json& output = result.result->final_output;
        if (output.is_string()) {
            platform->console->Printf("[AGENT] %s\n", output.get<std::string>().c_str());
        } else {
            platform->console->Printf("[AGENT] %s\n", output.dump().c_str());
        }
        platform->console->Printf("[STATS] Turns used: %d\n", result.result->turns);
    }

    return 0;
}
