# ForestHub SDK

[![CI](https://github.com/ForestHubAI/fh-sdk/actions/workflows/ci.yml/badge.svg)](https://github.com/ForestHubAI/fh-sdk/actions/workflows/ci.yml) [![License: AGPL-3.0](https://img.shields.io/badge/License-AGPL--3.0-blue.svg)](https://github.com/ForestHubAI/fh-sdk/blob/main/LICENSE) [![C++14](https://img.shields.io/badge/C%2B%2B-14-blue.svg)](https://isocpp.org/) [![PlatformIO](https://img.shields.io/badge/PlatformIO-Registry-orange.svg)](https://registry.platformio.org/libraries/foresthubai/fh-sdk) [![Release](https://img.shields.io/github/v/release/ForestHubAI/fh-sdk)](https://github.com/ForestHubAI/fh-sdk/releases)

A platform-agnostic C++14 framework for building LLM-powered applications, from cloud servers to microcontrollers. Write your code once and deploy it on PC (Linux/macOS/Windows) or embedded devices (ESP32, Portenta H7) without changing a line.

Built on a **Hardware Abstraction Layer (HAL)** that abstracts network, console, time, crypto, and GPIO -- the LLM SDK, agent framework, and RAG system run on top of it unchanged across platforms.

## Features

- **Platform-agnostic via HAL** -- same application code runs on PC and embedded targets; platform-specific implementations (CPR on PC, ArduinoHttpClient on ESP32) are injected at build time
- **Multi-provider LLM client** -- unified interface for ForestHub, OpenAI, Gemini, and Anthropic; routes requests to the right provider based on model ID
- **Agent framework** -- tool calling, multi-turn conversations, agent handoffs, and web search
- **RAG retriever** -- semantic document search via ForestHub backend
- **Embedded-safe** -- C++14, no exceptions, no RTTI; targets any C++14-capable toolchain including embedded compilers

## Installation

### PC (CMake)

Add to your project's `CMakeLists.txt`:

```cmake
include(FetchContent)
FetchContent_Declare(
    fh-sdk
    GIT_REPOSITORY https://github.com/ForestHubAI/fh-sdk.git
    GIT_TAG v0.1.1
)
FetchContent_MakeAvailable(fh-sdk)

target_link_libraries(your_app PRIVATE foresthub_core)
```

**Requirements:** CMake 3.14+, C++14 compiler (GCC 7+, Clang 5+, MSVC 2017+). Dependencies (CPR, nlohmann/json) are fetched automatically.

### Embedded (PlatformIO)

Add to your `platformio.ini`:

```ini
lib_deps =
    foresthubai/fh-sdk@^0.1.1
```

**Requirements:** PlatformIO CLI (`pip install platformio`).

See [Embedded Guide](https://github.com/ForestHubAI/fh-sdk/blob/main/docs/embedded.md) for detailed setup.

## Usage

### PC -- Agent with Tools

```cpp
#include "foresthub/agent/agent.hpp"
#include "foresthub/agent/runner.hpp"
#include "foresthub/client.hpp"
#include "foresthub/config/config.hpp"
#include "foresthub/core/input.hpp"
#include "foresthub/core/tools.hpp"
#include "foresthub/util/json.hpp"
#include "platform_setup.hpp"

using json = nlohmann::json;

// 1. Define tool: argument struct, deserializer, handler
struct WeatherArgs { std::string city; };
void from_json(const json& j, WeatherArgs& a) { a.city = j.value("city", ""); }
json GetWeather(const WeatherArgs& a) { return {{"temp", "22C"}, {"city", a.city}}; }

int main() {
    // 2. Platform + HTTP client via HAL
    auto platform = app::SetupPlatform();
    foresthub::platform::HttpClientConfig http_cfg;
    http_cfg.host = "fh-backend-368736749905.europe-west1.run.app";
    auto http_client = platform->CreateHttpClient(http_cfg);

    // 3. Configure provider + create client
    foresthub::config::ClientConfig cfg;
    foresthub::config::ProviderConfig fh_cfg;
    fh_cfg.base_url = "https://fh-backend-368736749905.europe-west1.run.app";
    fh_cfg.api_key = std::getenv("FORESTHUB_API_KEY");
    fh_cfg.supported_models = {"gpt-4.1", "gpt-4o"};
    cfg.remote.foresthub = fh_cfg;
    auto client = foresthub::Client::Create(cfg, http_client);

    // 4. Build agent with tool
    json schema = json::parse(R"({"properties":{"city":{"type":"string"}}})", nullptr, false);
    auto agent = std::make_shared<foresthub::agent::Agent>("weather-bot");
    agent->WithInstructions("You help with weather queries.")
         .WithOptions(foresthub::core::Options().WithTemperature(0.7f).WithMaxTokens(1024))
         .AddTool(foresthub::core::NewFunctionTool<WeatherArgs, json>(
             "get_weather", "Get current weather for a city", schema, GetWeather));

    // 5. Run agent
    auto runner = std::make_shared<foresthub::agent::Runner>(client, "gpt-4o");
    auto input = std::make_shared<foresthub::core::InputString>("Weather in Berlin?");
    foresthub::agent::RunResultOrError result = runner->Run(agent, input);
    if (!result.HasError()) {
        printf("%s\n", result.result->final_output.dump().c_str());
    }
}
```

### Embedded -- Chat on ESP32

The same SDK, but with Arduino `setup()`/`loop()` and explicit WiFi + time sync:

```cpp
#include <Arduino.h>
#include "env.hpp"  // WiFi credentials + API key
#include "foresthub/client.hpp"
#include "foresthub/config/config.hpp"
#include "foresthub/core/input.hpp"
#include "foresthub/core/types.hpp"
#include "foresthub/platform/platform.hpp"

static std::shared_ptr<foresthub::platform::PlatformContext> platform;

void setup() {
    // 1. Create platform (WiFi, Serial, NTP, TLS)
    foresthub::platform::PlatformConfig config;
    config.network.ssid = kWifiSsid;
    config.network.password = kWifiPassword;
    platform = foresthub::platform::CreatePlatform(config);

    // 2. Connect network + sync time (required for TLS)
    platform->console->Begin();
    platform->network->Connect();
    platform->time->SyncTime();

    // 3. Create client (same API as PC)
    foresthub::platform::HttpClientConfig http_cfg;
    http_cfg.host = "fh-backend-368736749905.europe-west1.run.app";
    auto http_client = platform->CreateHttpClient(http_cfg);

    foresthub::config::ClientConfig cfg;
    foresthub::config::ProviderConfig fh_cfg;
    fh_cfg.base_url = "https://fh-backend-368736749905.europe-west1.run.app";
    fh_cfg.api_key = kForesthubApiKey;
    fh_cfg.supported_models = {"gpt-4.1", "gpt-4.1-mini"};
    cfg.remote.foresthub = fh_cfg;
    auto client = foresthub::Client::Create(cfg, http_client);

    // 4. Send chat request
    foresthub::core::ChatRequest req;
    req.model = "gpt-4.1-mini";
    req.input = std::make_shared<foresthub::core::InputString>("What is the capital of France?");
    auto response = client->Chat(req);

    if (response) {
        platform->console->Printf("Response: %s\n", response->text.c_str());
    }
}

void loop() {
    platform->time->Delay(10000);
}
```

Note how steps 3-4 (client setup and chat request) are **identical** on both platforms -- only the platform initialization differs.

### Multi-Provider Support

Route requests to any supported provider -- the client selects the right one based on model ID:

| Provider | Config | Env Variable |
|----------|--------|-------------|
| ForestHub | `cfg.remote.foresthub = fh_cfg;` | `FORESTHUB_API_KEY` |
| OpenAI | `cfg.remote.openai = oai_cfg;` | `OPENAI_API_KEY` |
| Gemini | `cfg.remote.gemini = gem_cfg;` | `GEMINI_API_KEY` |
| Anthropic | `cfg.remote.anthropic = ant_cfg;` | `ANTHROPIC_API_KEY` |

See [Provider Guide](https://github.com/ForestHubAI/fh-sdk/blob/main/docs/providers.md) for detailed configuration.

For complete examples see [`examples/pc/`](https://github.com/ForestHubAI/fh-sdk/tree/main/examples/pc) and [`examples/embedded/`](https://github.com/ForestHubAI/fh-sdk/tree/main/examples/embedded).

## Documentation

- [Getting Started](https://github.com/ForestHubAI/fh-sdk/blob/main/docs/getting-started.md) -- Build and run your first chat
- [Provider Guide](https://github.com/ForestHubAI/fh-sdk/blob/main/docs/providers.md) -- ForestHub, OpenAI, Gemini, Anthropic configuration
- [Agent Framework](https://github.com/ForestHubAI/fh-sdk/blob/main/docs/agents.md) -- Tools, Runner, handoffs, options
- [RAG Retriever](https://github.com/ForestHubAI/fh-sdk/blob/main/docs/rag.md) -- Retrieval-augmented generation
- [HAL Architecture](https://github.com/ForestHubAI/fh-sdk/blob/main/docs/hal.md) -- Platform abstraction design and how to add new platforms
- [Embedded Platforms](https://github.com/ForestHubAI/fh-sdk/blob/main/docs/embedded.md) -- ESP32, Portenta H7, PlatformIO
- [API Reference](https://foresthubai.github.io/fh-sdk/) -- Generated from source

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

---

## Development

Everything below is for **contributors and maintainers** working on the SDK itself.

### Building from Source

```bash
# PC
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build -j4

# Embedded (pattern: pio run -d examples/embedded/<provider>/<example> -e esp32dev)
pio run -d pio/build_test -e esp32dev
```

See [Embedded Guide](https://github.com/ForestHubAI/fh-sdk/blob/main/docs/embedded.md) for detailed setup.

### Testing

```bash
cd build && ctest --output-on-failure
```

Run a specific test: `./build/bin/Debug/run_core_tests --gtest_filter="InputTest.*"`

Hand-rolled mocks in `tests/mocks/` (no GMock -- incompatible with `-fno-rtti`). See [THIRD_PARTY_NOTICES](https://github.com/ForestHubAI/fh-sdk/blob/main/THIRD_PARTY_NOTICES) for dependency license details.

## License

This project is dual-licensed:

- **Open Source**: [GNU Affero General Public License v3.0](https://github.com/ForestHubAI/fh-sdk/blob/main/LICENSE) (AGPL-3.0-only)
- **Commercial**: For commercial licensing options, contact root@foresthub.ai

By using this software, you agree to the terms of the AGPL-3.0 license unless you have obtained a separate commercial license.
