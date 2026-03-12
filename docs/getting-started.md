# Getting Started

This guide walks you through building the ForestHub SDK from source and sending your first chat request and agent workflow.

## Prerequisites

- **C++ compiler** with C++14 support (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake** 3.14 or later
- **Internet connection** (CMake fetches CPR and GoogleTest automatically)
- An API key from at least one supported provider (ForestHub, OpenAI, Gemini, or Anthropic)

For embedded development, see the [Embedded Platforms](https://github.com/ForestHubAI/fh-sdk/blob/main/docs/embedded.md) guide instead.

## Installation

Clone the repository and build with CMake:

```bash
git clone https://github.com/ForestHubAI/fh-sdk.git
cd fh-sdk
cmake -S . -B build
cmake --build build -j4
```

To also build the test suite:

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build -j4
cd build && ctest --output-on-failure
```

## Your First Chat

The simplest use case is sending a single prompt to an LLM and printing the response. This example uses the ForestHub provider, but any provider follows the same pattern (see the [Provider Guide](https://github.com/ForestHubAI/fh-sdk/blob/main/docs/providers.md)).

```cpp
#include "foresthub/client.hpp"
#include "foresthub/config/config.hpp"
#include "foresthub/core/input.hpp"
#include "foresthub/core/types.hpp"

int main() {
    // Configure the ForestHub provider
    foresthub::config::ProviderConfig fh_cfg;
    fh_cfg.api_key = std::getenv("FORESTHUB_API_KEY");
    fh_cfg.supported_models = {"gpt-4.1", "gpt-4.1-mini"};

    foresthub::config::ClientConfig cfg;
    cfg.remote.foresthub = fh_cfg;

    // Create client (http_client=nullptr uses the built-in default on PC)
    std::unique_ptr<foresthub::Client> client = foresthub::Client::Create(cfg);

    // Build and send a chat request
    foresthub::core::ChatRequest req;
    req.model = "gpt-4.1-mini";
    req.input = std::make_shared<foresthub::core::InputString>("What is the capital of France?");
    req.WithSystemPrompt("You are a helpful geography assistant.");

    std::shared_ptr<foresthub::core::ChatResponse> response = client->Chat(req);
    if (response) {
        printf("Response: %s\n", response->text.c_str());
        printf("Tokens used: %d\n", response->tokens_used);
    }
}
```

> See [`examples/pc/foresthub/chat.cpp`](https://github.com/ForestHubAI/fh-sdk/blob/main/examples/pc/foresthub/chat.cpp) for the full example with platform setup and error handling.

## Your First Agent

An agent combines instructions, tools, and generation options into a reusable unit. The `Runner` executes the multi-turn loop, automatically dispatching tool calls and feeding results back to the LLM.

```cpp
#include "foresthub/agent/agent.hpp"
#include "foresthub/agent/runner.hpp"
#include "foresthub/core/tools.hpp"
#include "foresthub/util/json.hpp"

using json = nlohmann::json;

// 1. Define tool arguments and handler
struct WeatherArgs { std::string city; };
void from_json(const json& j, WeatherArgs& a) { a.city = j.value("city", ""); }
json GetWeather(const WeatherArgs& a) { return {{"temp", "22C"}, {"city", a.city}}; }

// 2. Create the tool with a JSON schema
json schema = json::parse(
    R"({"type":"object","properties":{"city":{"type":"string"}},"required":["city"]})",
    nullptr, false);

auto tool = foresthub::core::NewFunctionTool<WeatherArgs, json>(
    "get_weather", "Returns current weather for a city.", schema, GetWeather);

// 3. Configure the agent
auto agent = std::make_shared<foresthub::agent::Agent>("WeatherBot");
agent->WithInstructions("You help with weather queries. Use the provided tool.")
     .WithOptions(foresthub::core::Options().WithTemperature(0.7f).WithMaxTokens(1024))
     .AddTool(tool);

// 4. Run the agent
auto runner = std::make_shared<foresthub::agent::Runner>(client, "gpt-4.1-mini");
runner->WithMaxTurns(5);

auto input = std::make_shared<foresthub::core::InputString>("How is the weather in Berlin?");
foresthub::agent::RunResultOrError result = runner->Run(agent, input);

if (!result.HasError()) {
    printf("Agent: %s\n", result.result->final_output.dump().c_str());
    printf("Turns: %d\n", result.result->turns);
}
```

> See [`examples/pc/foresthub/agent.cpp`](https://github.com/ForestHubAI/fh-sdk/blob/main/examples/pc/foresthub/agent.cpp) for the full example.

## Configuration

### ClientConfig

The top-level `ClientConfig` controls which providers the `Client` will create:

```cpp
foresthub::config::ClientConfig cfg;
cfg.remote.foresthub = fh_cfg;    // ForestHub backend
cfg.remote.openai = oai_cfg;      // Direct OpenAI
cfg.remote.gemini = gemini_cfg;   // Direct Google Gemini
cfg.remote.anthropic = anth_cfg;  // Direct Anthropic Claude
```

Each field in `cfg.remote` is a `foresthub::Optional<ProviderConfig>`. A provider is only created when its config has a value.

### ProviderConfig

Every provider uses the same config struct:

```cpp
foresthub::config::ProviderConfig cfg;
cfg.api_key = "your-api-key";                   // Required
cfg.base_url = "https://custom.endpoint.com";   // Optional (provider default if empty)
cfg.supported_models = {"model-a", "model-b"};  // Models routed to this provider
```

### Environment Variables

The examples read API keys from environment variables. Export them before running:

```bash
export FORESTHUB_API_KEY=your_foresthub_key
export OPENAI_API_KEY=your_openai_key
export GEMINI_API_KEY=your_gemini_key
export ANTHROPIC_API_KEY=your_anthropic_key
```

## Next Steps

- [Provider Guide](https://github.com/ForestHubAI/fh-sdk/blob/main/docs/providers.md) -- configure ForestHub, OpenAI, Gemini, and Anthropic
- [Agent Framework](https://github.com/ForestHubAI/fh-sdk/blob/main/docs/agents.md) -- tools, multi-turn execution, structured output, and handoffs
- [RAG Guide](https://github.com/ForestHubAI/fh-sdk/blob/main/docs/rag.md) -- retrieval-augmented generation with document search
- [Embedded Platforms](https://github.com/ForestHubAI/fh-sdk/blob/main/docs/embedded.md) -- deploy on ESP32, Portenta H7, and other MCUs
