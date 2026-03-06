// =============================================================================
// Anthropic Embedded Agent Example
// =============================================================================
// Agent with tool calling on Arduino (ESP32, Portenta H7).
// Creates a WeatherBot agent that uses a get_weather tool, sends a prompt,
// and prints the multi-turn conversation result via the console HAL.
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
#include "foresthub/core/options.hpp"
#include "foresthub/core/tools.hpp"
#include "foresthub/platform/platform.hpp"
#include "foresthub/util/json.hpp"

using json = nlohmann::json;

// -----------------------------------------------------------------------------
// Weather Tool
// -----------------------------------------------------------------------------

struct WeatherArgs {
    std::string city;
};

void from_json(const json& j, WeatherArgs& args) {
    args.city = j.value("city", "");
}

json GetWeather(const WeatherArgs& args) {
    const std::string& city = args.city;

    if (city == "Berlin") return json{{"city", "Berlin"}, {"temperature", "15C"}, {"condition", "cloudy"}};
    if (city == "London") return json{{"city", "London"}, {"temperature", "12C"}, {"condition", "light rain"}};
    if (city == "Madrid") return json{{"city", "Madrid"}, {"temperature", "28C"}, {"condition", "sunny"}};
    if (city == "Paris") return json{{"city", "Paris"}, {"temperature", "18C"}, {"condition", "partly sunny"}};

    return json{
        {"error", "Weather data for city '" + city + "' not available. Available: Berlin, London, Madrid, Paris."}};
}

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
    platform->console->Printf("=== Anthropic Embedded Agent ===\n\n");
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

    // 8. Create weather tool
    json weather_tool_schema = json::parse(
        R"({"type":"object","properties":{"city":{"type":"string","description":"The city to get the weather for (e.g. Berlin, London)."}},"required":["city"]})",
        nullptr, false);

    auto weather_tool = foresthub::core::NewFunctionTool<WeatherArgs, json>(
        "get_weather", "Returns the current weather for a city.", weather_tool_schema, GetWeather);

    // 9. Create agent
    auto agent = std::make_shared<foresthub::agent::Agent>("WeatherBot");
    agent->WithInstructions("You are a helpful assistant. If asked about weather, use the provided tool.")
        .WithOptions(foresthub::core::Options().WithTemperature(0.7f).WithMaxTokens(1024))
        .AddTool(weather_tool);

    // 10. Create runner and execute
    std::string model_name = "claude-sonnet-4-6";

    if (!client->SupportsModel(model_name)) {
        platform->console->Printf("[ERROR] Model '%s' not supported.\n", model_name.c_str());
        platform->console->Flush();
        while (true) {
            platform->time->Delay(1000);
        }
    }

    auto runner = std::make_shared<foresthub::agent::Runner>(client, model_name);
    runner->WithMaxTurns(5);

    std::string prompt = "How is the weather in Madrid and how does it look in Berlin?";
    platform->console->Printf("[USER] %s\n", prompt.c_str());
    platform->console->Printf("[INFO] Running agent...\n");

    auto input = std::make_shared<foresthub::core::InputString>(prompt);
    foresthub::agent::RunResultOrError result = runner->Run(agent, input);

    // 11. Print result
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

    platform->console->Printf("\n[DONE]\n");
    platform->console->Flush();
}

void loop() {
    // One-shot example — nothing to do in loop
    platform->time->Delay(10000);
}
