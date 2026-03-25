# Embedded Platforms

The ForestHub SDK runs on microcontrollers through the PlatformIO build system and a Hardware Abstraction Layer (HAL) that provides network, console, time, and crypto services.

## Supported Boards

| Board | Platform | Tested |
|-------|----------|--------|
| ESP32 (esp32dev) | espressif32 | Yes |
| Arduino Portenta H7 (M7 core) | ststm32 | Yes |

Other Arduino-compatible boards with sufficient Flash and RAM may also work.

## Prerequisites

- [PlatformIO CLI](https://docs.platformio.org/en/latest/core/installation.html) (`pip install platformio`)
- A supported board connected via USB
- WiFi network credentials
- An API key from at least one provider

## Project Setup

Create a `platformio.ini` file in your project directory:

```ini
[platformio]
default_envs = esp32dev

[env]
framework = arduino
build_unflags = -std=gnu++11
build_flags =
    -std=gnu++14
    -DFORESTHUB_ENABLE_NETWORK
    -DFORESTHUB_ENABLE_CRYPTO
    -DFORESTHUB_ENABLE_GPIO
lib_deps =
    foresthub/fh-sdk
monitor_speed = 115200

[env:esp32dev]
platform = espressif32
board = esp32dev
board_build.partitions = min_spiffs.csv
build_flags =
    ${env.build_flags}
    -DCORE_DEBUG_LEVEL=0

[env:portenta_h7_m7]
platform = ststm32
board = portenta_h7_m7
build_flags =
    ${env.build_flags}
    -Wl,--allow-multiple-definition
```

Key points:

- C++14 is required (`-std=gnu++14` replaces the Arduino default of C++11).
- The `FORESTHUB_ENABLE_*` macros control which HAL subsystems are compiled in.
- Use `board_build.partitions = min_spiffs.csv` on ESP32 to maximize available Flash.

## WiFi Configuration

Embedded examples use a header/source pair to separate credentials from code:

**include/env.hpp**

```cpp
#ifndef EXAMPLE_ENV_HPP
#define EXAMPLE_ENV_HPP

extern const char* kWifiSsid;
extern const char* kWifiPassword;
extern const char* kForesthubApiKey;

#endif
```

**src/env.cpp** (gitignored -- copy from `env.cpp.example`)

```cpp
#include "env.hpp"

const char* kWifiSsid = "YourSSID";
const char* kWifiPassword = "YourPassword";
const char* kForesthubApiKey = "YourApiKey";
```

Platform initialization uses `PlatformConfig` to pass network credentials:

```cpp
foresthub::platform::PlatformConfig config;
config.network.ssid = kWifiSsid;
config.network.password = kWifiPassword;
auto platform = foresthub::platform::CreatePlatform(config);
```

## Optional Subsystems

HAL subsystems are opt-in via compile-time macros. Console and Time are always available. The remaining subsystems are enabled by adding flags to `build_flags`:

| Macro | Subsystem | Purpose |
|-------|-----------|---------|
| `FORESTHUB_ENABLE_NETWORK` | WiFi + HTTP client | Required for any LLM API calls |
| `FORESTHUB_ENABLE_CRYPTO` | TLS/HTTPS (mbedTLS) | Required for HTTPS connections |
| `FORESTHUB_ENABLE_GPIO` | Digital/analog/PWM I/O | Pin control for sensors and actuators |

Omitting unused subsystems saves significant Flash:

| Configuration | ESP32 Flash | Portenta Flash |
|---------------|-------------|----------------|
| Full (all subsystems) | 908 KB | 473 KB |
| Minimal (Console + Time only) | 389 KB | 270 KB |
| **Savings** | **519 KB (57%)** | **203 KB (43%)** |

For a minimal build that only needs Console and Time (no networking), omit all three macros.

## Entry Point Pattern

Arduino sketches use `setup()` and `loop()` instead of `main()`. The typical pattern initializes the platform in `setup()` and runs the LLM interaction there for one-shot examples:

```cpp
#include <Arduino.h>
#include "env.hpp"
#include "foresthub/client.hpp"
#include "foresthub/config/config.hpp"
#include "foresthub/llm/input.hpp"
#include "foresthub/llm/types.hpp"
#include "foresthub/hal/platform.hpp"

static std::shared_ptr<foresthub::platform::Platform> platform;

void setup() {
    // 1. Create platform context
    foresthub::platform::PlatformConfig config;
    config.network.ssid = kWifiSsid;
    config.network.password = kWifiPassword;
    platform = foresthub::platform::CreatePlatform(config);

    // 2. Initialize console
    platform->console->Begin();

    // 3. Connect network (with retry)
    std::string net_err = platform->network->Connect();
    if (!net_err.empty()) { /* handle error */ }

    // 4. Synchronize time (required for TLS certificate validation)
    std::string time_err = platform->time->SyncTime();
    if (!time_err.empty()) { /* handle error */ }

    // 5. Create HTTP client and send requests (same API as PC)
    foresthub::platform::HttpClientConfig http_cfg;
    http_cfg.host = "fh-backend-368736749905.europe-west1.run.app";
    auto http_client = platform->CreateHttpClient(http_cfg);

    // ... configure client and send chat requests ...
}

void loop() {
    platform->time->Delay(10000);
}
```

The SDK API for configuring providers, building requests, and calling `Client::Chat()` is identical on PC and embedded. Only the platform initialization differs.

## Building and Flashing

Build and upload from the command line:

```bash
# Build only
pio run -d path/to/project -e esp32dev

# Build and upload
pio run -d path/to/project -e esp32dev -t upload

# Monitor serial output
pio device monitor -b 115200

# Portenta H7
pio run -d path/to/project -e portenta_h7_m7
```

## Example Projects

The repository includes ready-to-use embedded examples:

| Example | Command |
|---------|---------|
| ForestHub chat | `pio run -d examples/embedded/foresthub/chat -e esp32dev` |
| ForestHub agent | `pio run -d examples/embedded/foresthub/agent -e esp32dev` |
| ForestHub RAG | `pio run -d examples/embedded/foresthub/rag -e esp32dev` |
| ForestHub websearch | `pio run -d examples/embedded/foresthub/websearch -e esp32dev` |
| OpenAI chat | `pio run -d examples/embedded/openai/chat -e esp32dev` |
| OpenAI agent | `pio run -d examples/embedded/openai/agent -e esp32dev` |
| OpenAI websearch | `pio run -d examples/embedded/openai/websearch -e esp32dev` |
| Gemini chat | `pio run -d examples/embedded/gemini/chat -e esp32dev` |
| Gemini agent | `pio run -d examples/embedded/gemini/agent -e esp32dev` |
| Gemini websearch | `pio run -d examples/embedded/gemini/websearch -e esp32dev` |
| Anthropic chat | `pio run -d examples/embedded/anthropic/chat -e esp32dev` |
| Anthropic agent | `pio run -d examples/embedded/anthropic/agent -e esp32dev` |
| Anthropic websearch | `pio run -d examples/embedded/anthropic/websearch -e esp32dev` |

Each example includes an `env.cpp.example` file. Copy it to `env.cpp` and fill in your credentials before building.

## Troubleshooting

### WiFi connection fails

- Verify SSID and password in `env.cpp`. Note that credentials are case-sensitive.
- Ensure the board is within range of the access point.
- Try increasing the connection timeout: `platform->network->Connect(60000)` (60 seconds).

### TLS / HTTPS failures

- Ensure `FORESTHUB_ENABLE_CRYPTO` is set in `build_flags`.
- Call `platform->time->SyncTime()` before making HTTPS requests. TLS certificate validation requires accurate system time.
- On ESP32, use `board_build.partitions = min_spiffs.csv` to ensure enough Flash for the TLS stack.

### Watchdog timer (WDT) resets

- LLM API calls can take several seconds. If your board has a hardware watchdog with a short timeout, increase it or feed the watchdog between operations.
- Use `platform->time->Delay()` instead of Arduino's `delay()` to keep the HAL consistent.

### Flash size exceeded

- Remove unused subsystem macros from `build_flags` (see the Flash savings table above).
- Use `min_spiffs.csv` partition scheme on ESP32.
- Consider using the ForestHub backend provider instead of direct provider access, as it requires fewer compiled code paths.

## Further Reading

- [HAL Architecture](https://github.com/ForestHubAI/fh-sdk/blob/main/docs/hal.md) -- Platform abstraction design and how to add new platforms
- [Getting Started](https://github.com/ForestHubAI/fh-sdk/blob/main/docs/getting-started.md) -- PC build and first steps
- [Provider Guide](https://github.com/ForestHubAI/fh-sdk/blob/main/docs/providers.md) -- configure each LLM provider
- [Agent Framework](https://github.com/ForestHubAI/fh-sdk/blob/main/docs/agents.md) -- tools and multi-turn agent workflows
