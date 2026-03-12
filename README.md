# ForestHub SDK

C++14 LLM SDK with unified multi-provider interface. Supports ForestHub backend, direct OpenAI (Responses API), direct Google Gemini (generateContent API), and direct Anthropic Claude (Messages API). Designed for PC and embedded platforms (MCUs).

## Features

- **Multi-provider routing** -- automatic request routing to the right LLM provider based on model ID
- **Agent framework** -- tool calling, multi-turn conversations, agent handoffs, generation parameters (temperature, max_tokens), and built-in web search
- **RAG retriever** -- semantic document search via ForestHub backend with XML context formatting for LLM prompt injection
- **Internal tools** -- provider-managed tools like web search with query logging via `InternalToolCall`
- **Platform-agnostic** -- runs on PC (Linux/macOS) and embedded platforms via Hardware Abstraction Layer (Arduino as first implementation)
- **No exceptions** -- embedded-safe design using string error returns and result structs
- **No RTTI** -- all polymorphic dispatch via virtual methods and type enums (no dynamic_cast)
- **Google Style `struct`/`class` convention** -- `struct` for passive data (DTOs, configs), `class` for types with behavior (interfaces, implementations with virtual methods or private state)
- **Automatic schema handling** -- minimal JSON Schema input auto-normalized and strictified per provider requirements
- **Fluent builder API** -- method chaining for requests, agents, and configuration
- **C++14 compatible** -- targets any C++14-capable toolchain (GCC 7+, Clang 5+, MSVC 2017+, embedded compilers)

## Quick Start

### Prerequisites

- **PC**: CMake 3.14+, C++14 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- **Embedded**: PlatformIO CLI (`pip install platformio`)
- Internet connection (dependencies fetched automatically)

### Build

```bash
cmake -S . -B build
cmake --build build -j4
```

### Run

```bash
export FORESTHUB_API_KEY=your_key
./build/foresthub_chat
```

## Usage

### Simple Chat

```cpp
#include "foresthub/client.hpp"
#include "foresthub/config/config.hpp"
#include "foresthub/core/input.hpp"
#include "foresthub/core/types.hpp"
#include "platform_setup.hpp"

int main() {
    std::shared_ptr<foresthub::platform::PlatformContext> platform = app::SetupPlatform();

    foresthub::platform::HttpClientConfig http_cfg;
    http_cfg.host = "fh-backend-368736749905.europe-west1.run.app";
    std::shared_ptr<foresthub::core::HttpClient> http_client =
        platform->CreateHttpClient(http_cfg);

    foresthub::config::ClientConfig cfg;
    foresthub::config::ProviderConfig fh_cfg;
    fh_cfg.base_url = "https://fh-backend-368736749905.europe-west1.run.app";
    fh_cfg.api_key = std::getenv("FORESTHUB_API_KEY");
    fh_cfg.supported_models = {"gpt-4.1", "gpt-4.1-mini"};
    cfg.remote.foresthub = fh_cfg;

    std::shared_ptr<foresthub::Client> client = foresthub::Client::Create(cfg, http_client);

    foresthub::core::ChatRequest req;
    req.model = "gpt-4.1-mini";
    req.input = std::make_shared<foresthub::core::InputString>("Hello!");
    req.WithSystemPrompt("You are a helpful assistant.");

    std::shared_ptr<foresthub::core::ChatResponse> response = client->Chat(req);
    if (response) {
        printf("%s\n", response->text.c_str());
    }
}
```

### Agent with Tools

```cpp
#include "foresthub/agent/agent.hpp"
#include "foresthub/agent/runner.hpp"
#include "foresthub/core/tools.hpp"

// Define tool arguments + handler
struct WeatherArgs { std::string city; };
void from_json(const json& j, WeatherArgs& a) { a.city = j.value("city", ""); }
json get_weather(const WeatherArgs& a) { return {{"temp", "22C"}, {"city", a.city}}; }

// Create agent with tool
foresthub::agent::Agent agent("weather-bot");
agent.WithInstructions("You help with weather queries.")
     .WithOptions(foresthub::core::Options().WithTemperature(0.7f).WithMaxTokens(1024))
     .AddTool(foresthub::core::NewFunctionTool<WeatherArgs, json>(
         "get_weather", "Get current weather", schema, get_weather));

// Run agent
foresthub::agent::Runner runner(client, "gpt-4.1-mini");
foresthub::agent::RunResultOrError result = runner.Run(
    std::make_shared<foresthub::agent::Agent>(agent),
    std::make_shared<foresthub::core::InputString>("What's the weather in Berlin?"));
if (!result.HasError()) {
    printf("%s\n", result.result->final_output.dump().c_str());
}
```

See `examples/pc/foresthub/chat.cpp` and `examples/pc/foresthub/agent.cpp` for complete examples. For web search, see `examples/pc/foresthub/websearch.cpp`. For structured output (JSON schema), see `examples/pc/foresthub/structured_output.cpp`. For RAG (retrieval-augmented generation), see `examples/pc/foresthub/rag.cpp`. For direct OpenAI usage, see `examples/pc/openai/`. For direct Gemini usage, see `examples/pc/gemini/`. For direct Anthropic Claude usage, see `examples/pc/anthropic/`.

> **Note:** Web search is not currently supported for the Anthropic provider due to multi-turn conversation requirements. See the AnthropicProvider header documentation for details.

## Architecture

```
Application Layer        -- examples/pc/, examples/embedded/
        |
        v
HAL (foresthub::platform) -- Network, Console, Time, Crypto
        |
        v
Core (foresthub::core)    -- LLM client, Agent framework, Tools
```

The **Core** layer defines abstract interfaces (e.g., `HttpClient`). The **HAL** layer provides platform-specific implementations (CPR on PC, ArduinoHttpClient on ESP32). Applications create a `PlatformConfig` (with WiFi credentials on embedded) and call `CreatePlatform(config)` to initialize the platform, then inject dependencies into Core.

### Optional Subsystems

HAL subsystems are opt-in via compile-time macros. Add to your `platformio.ini`:

```ini
build_flags =
    -DFORESTHUB_ENABLE_NETWORK    ; WiFi + HTTP client
    -DFORESTHUB_ENABLE_CRYPTO     ; TLS/HTTPS support
    -DFORESTHUB_ENABLE_GPIO       ; Digital/analog/PWM pin I/O
```

Console and Time are always available. Omitting macros saves significant Flash:

| Configuration | ESP32 Flash | Portenta Flash |
|---|---|---|
| Full (all subsystems) | 908 KB | 473 KB |
| Minimal (Console+Time) | 389 KB | 270 KB |
| **Savings** | **519 KB (57%)** | **203 KB (43%)** |

## Building

### PC (CMake)

| Target | Command |
|--------|---------|
| Standard | `cmake -S . -B build && cmake --build build` |
| With tests | `cmake -S . -B build -DBUILD_TESTING=ON && cmake --build build` |

### Embedded (PlatformIO)

| Target | Command |
|--------|---------|
| Full build (all subsystems) | `pio run -d pio/build_test -e esp32dev` |
| Minimal build (Console+Time) | `pio run -d pio/build_test -e esp32_none` |
| ForestHub chat | `pio run -d examples/embedded/foresthub/chat -e esp32dev` |
| ForestHub agent | `pio run -d examples/embedded/foresthub/agent -e esp32dev` |
| ForestHub RAG | `pio run -d examples/embedded/foresthub/rag -e esp32dev` |
| OpenAI chat | `pio run -d examples/embedded/openai/chat -e esp32dev` |
| OpenAI agent | `pio run -d examples/embedded/openai/agent -e esp32dev` |
| Gemini chat | `pio run -d examples/embedded/gemini/chat -e esp32dev` |
| Gemini agent | `pio run -d examples/embedded/gemini/agent -e esp32dev` |
| Anthropic chat | `pio run -d examples/embedded/anthropic/chat -e esp32dev` |
| Anthropic agent | `pio run -d examples/embedded/anthropic/agent -e esp32dev` |
| HTTP/HTTPS test | `pio run -d examples/embedded/http_test -e esp32dev` |
| HTTP-only test (no Crypto) | `pio run -d examples/embedded/http_test -e esp32_http_only` |
| Ticker example | `pio run -d examples/embedded/ticker -e esp32dev` |
| Portenta H7 | `pio run -d pio/build_test -e portenta_h7_m7` |

ForestHub examples require `FORESTHUB_API_KEY`, OpenAI examples require `OPENAI_API_KEY`, Gemini examples require `GEMINI_API_KEY`, Anthropic examples require `ANTHROPIC_API_KEY`.

## Testing

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build -j4
cd build && ctest --output-on-failure
```

~422 tests across 8 executables:

| Executable | Tests | Scope |
|------------|-------|-------|
| `run_core_tests` | 101 | Input, model, options, tools, types, json, client |
| `run_agent_tests` | 42 | Agent construction, Runner execution loop, options |
| `run_provider_tests` | ~149 | ForestHub + OpenAI + Gemini + Anthropic HTTP, retry, errors, schema strictification |
| `run_platform_tests` | 44 | PC platform factory, subsystems, GPIO, ENABLE macros, timezone, console |
| `run_rag_tests` | 15 | RemoteRetriever HTTP, retry, JSON, serialization, FormatContext |
| `run_integration_tests` | 7 | Runner-Provider chain, Client routing |
| `run_contract_tests` | 12 | ForestHub API JSON schema verification |
| `run_util_tests` | 50 | Optional polyfill, Ticker, Schema normalization |

Hand-rolled mocks in `tests/mocks/` (no GMock -- incompatible with `-fno-rtti`).

## Project Structure

```
include/foresthub/        Public API headers
  agent/                  Agent framework (agent, runner, handoff)
  config/                 Configuration structs
  core/                   Core abstractions (provider, tools, types, input)
  platform/               HAL interfaces (network, console, time, crypto)
  provider/remote/        Provider implementations (Anthropic, ForestHub, Gemini, OpenAI)
  rag/                    RAG module (retriever interface, types, remote retriever)
  util/                   Utilities (Optional<T> polyfill, JSON wrapper, Ticker, Schema normalization)
src/                      Implementation
  provider/remote/        Provider implementations (anthropic/, foresthub/, gemini/, openai/)
  rag/                    RAG module (serialization, remote/retriever)
  platform/pc/            PC implementations (CPR, stdin/stdout, std::chrono)
  platform/arduino/       Arduino implementations (WiFi, Serial, NTP)
  platform/common/        Shared platform code (TLS certs)
examples/
  pc/                     PC examples (anthropic/, foresthub/, gemini/, openai/ -- chat + agent + websearch + structured_output + rag)
  embedded/               Arduino examples (anthropic/, foresthub/, gemini/, openai/ -- PlatformIO projects per provider + rag)
                          Standalone: blink/, http_test/, ticker/
tests/                    GoogleTest suites (~422 tests)
scripts/                  Build and coverage scripts
  core/                   Core tests (input, model, options, tools, types, json, client)
  agent/                  Agent framework tests (agent, runner)
  provider/               Provider tests (Anthropic + ForestHub + OpenAI + Gemini HTTP, retry, errors)
  rag/                    RAG tests (RemoteRetriever HTTP, retry, JSON, serialization)
  platform/               Platform tests (PC factory, subsystems)
  integration/            Integration tests (Runner-Provider, Client routing)
  contract/               Contract tests (ForestHub API JSON schema)
  util/                   Utility tests (Optional polyfill, Ticker)
  mocks/                  Hand-rolled mocks (HttpClient, Provider, LLMClient)
third_party/              Vendored dependencies (nlohmann/json)
library.json              PlatformIO library manifest
pio/build_test/           PlatformIO build verification (ESP32, Portenta H7)
```

## Dependencies

| Dependency | Version | Purpose | Source |
|------------|---------|---------|--------|
| nlohmann/json | 3.12.0 | JSON parsing and serialization | Vendored (`third_party/`), MIT license |
| CPR | 1.9.9 | HTTP client for PC builds | CMake FetchContent |
| GoogleTest | 1.17.0 | Unit testing framework | CMake FetchContent |
| ArduinoHttpClient | >=0.6.1 | HTTP client for Arduino builds (auto-resolved, framework-gated) | PlatformIO (`library.json`) |

See [THIRD_PARTY_NOTICES](THIRD_PARTY_NOTICES) for full license details of all third-party components.

## License

This project is dual-licensed:

- **Open Source**: [GNU Affero General Public License v3.0](LICENSE) (AGPL-3.0-only)
- **Commercial**: For commercial licensing options, contact root@foresthub.ai

By using this software, you agree to the terms of the AGPL-3.0 license unless you have obtained a separate commercial license.
