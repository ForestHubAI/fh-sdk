# Architecture — fh-sdk

## Project Overview

C++14 LLM SDK with unified interface for multiple providers (ForestHub, OpenAI, Gemini, Anthropic). Platform-agnostic via HAL and dependency injection — targets PC (CMake) and embedded/Arduino (PlatformIO). Agent framework with tool-calling and multi-turn conversations.

## Strict Architectural Rules

1. **No Exceptions** — Library targets embedded (Arduino).
   - Return `std::string` for errors (empty = success). Use `foresthub::util::Optional<T>` for optional fields.
   - JSON: Always `nlohmann::json::parse(raw, nullptr, false)` + check `.is_discarded()`.
   - Always include via `#include "foresthub/util/json.hpp"`, never `<nlohmann/json.hpp>` directly.

2. **No RTTI** — Embedded defaults to `-fno-rtti`.
   - Never use `dynamic_cast`, `dynamic_pointer_cast`, or `typeid`.
   - Use `toolType()`/`inputType()` enum + `std::static_pointer_cast` for dispatch.
   - Use virtual `toJson(json&)` for polymorphic serialization.

3. **Core/HAL Separation** — Core (`foresthub::core`) defines dependencies (e.g., `HttpClient`). HAL (`foresthub::platform`) provides infrastructure. **Core must be completely unaware of HAL.**

4. **Platform Isolation** — `include/foresthub/`: NEVER include `<Arduino.h>`. Public API is 100% platform-agnostic. Platform-specific code only in `src/platform/arduino/*.cpp` and `examples/embedded/`.
   - Entry points: PC uses `int main()`, Arduino uses `setup()`/`loop()` (with `#include <Arduino.h>` in sketch only)
   - Translation Unit Guards: `#ifdef FORESTHUB_PLATFORM_PC` (CMake) / `#ifdef FORESTHUB_PLATFORM_ARDUINO` (PlatformIO)
   - Optional Subsystems: `FORESTHUB_ENABLE_NETWORK`, `FORESTHUB_ENABLE_CRYPTO`, `FORESTHUB_ENABLE_GPIO`

5. **`struct` vs `class`** (Google Style):
   - `struct`: Passive data (DTOs, configs) — `ChatRequest`, `Options`, `PlatformConfig`, `RunResult`
   - `class`: Behavior/invariants — interfaces, implementations, polymorphic types

## Code Style

**ALL code MUST follow [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) and be strictly C++14 compliant.** No C++17 features (`<optional>`, `<string_view>`, `<any>`, `if constexpr`, structured bindings, `std::variant`). Use `foresthub::util::Optional<T>` instead of `std::optional`.

4-space indent, 120-char line limit. See `.clang-format` and `.clang-tidy`.

**Naming:**
| Element | Convention | Examples |
|---------|-----------|----------|
| Classes/Structs/Methods | PascalCase | `ChatRequest`, `AddTool()` |
| Getters/Accessors | snake_case | `tool_name()`, `call_id()` |
| Variables/Params/Struct members | snake_case | `base_url`, `api_key` |
| Private class members | snake_case_ | `timeout_ms_`, `start_time_` |
| Constants/Enum values | kPascalCase | `kPlainText`, `kConnected` |
| Namespaces | lowercase | `foresthub::core` |
| Macros | SCREAMING_CASE | `FORESTHUB_PLATFORM_PC` |
| Files | snake_case | `http_client.cpp` |

**Type deduction (`auto`):** Only when type is obvious (factories, casts, iterators). Explicit types otherwise.

**Smart pointers:** `std::shared_ptr` for polymorphic boundaries, `std::unique_ptr` for ownership. Prefer `std::make_shared`/`std::make_unique`.

## Documentation Rules

| Location | Format | Content |
|----------|--------|---------|
| `include/foresthub/` (Public API) | `///` + `@param`/`@return` | What + How to use (Doxygen) |
| `src/*.hpp` (Impl headers) | `///` one-liners | What makes this impl special |
| `src/*.cpp` (Impl source) | Plain `//` | How + Why (Google Style) |

- Use `///` (not `/** */`), `@param`/`@return` (not `\param`). No `@brief`.
- Omit `@param`/`@return` when obvious. Builder methods (`With*`/`Add*`) omit `@return`.
- Platform interfaces (`include/foresthub/hal/`): Hardware-neutral — never mention specific hardware platforms.

## Architecture

**Directory Map:** `include/foresthub/` (Public API headers), `src/` (Core + Provider impls), `src/platform/{pc,arduino}/` (HAL impls), `examples/{pc,embedded}/` (entry points), `tests/` (GoogleTest suites), `third_party/` (vendored deps).

**Core Abstractions:**
- `Provider` (`foresthub/core/provider.hpp`): LLMClient base with `Chat()` — `ForestHubProvider`, `OpenAIProvider`, `GeminiProvider`, `AnthropicProvider`
- `Client` (`foresthub/client.hpp`): Multi-provider routing by model ID. Factory: `Client::Create(cfg, http_client)`
- `Agent` (`foresthub/agent/`): Agent with instructions, tools, and generation options. `Runner` executes multi-turn loops. Agent handoffs via `Agent::AsTool()`
- `Tool` (`foresthub/core/tools.hpp`): `ExternalTool` (user-defined), `FunctionTool` (C++ callback), `InternalToolCall` (provider-handled, e.g. `WebSearch`)
- `Input` (`foresthub/core/input.hpp`): Polymorphic — `InputString` (text) or `InputItems` (conversation history)
- `ChatRequest`/`ChatResponse` (`foresthub/core/types.hpp`): Fluent API with `WithSystemPrompt()`, `AddTool()`, etc.
- `Retriever` (`foresthub/rag/`): `Retriever` interface — `RemoteRetriever` (POST /rag/query). Types: `QueryRequest`, `QueryResult`, `QueryResponse`. Helper: `FormatContext()` (XML)
- `HAL` (`foresthub/hal/`): `Platform` factory — Network, Console, Time, Crypto, GPIO interfaces
- `Schema` (`foresthub/util/schema.hpp`): `NormalizeSchema()` wraps minimal property maps into full JSON Schema. Provider-specific pipelines in `src/provider/remote/schema_utils.{hpp,cpp}`
- Provider impls: `src/provider/remote/{anthropic,foresthub,gemini,openai}/` (each has `provider.cpp` + `mapping.{hpp,cpp}`)

**Data Flow:**
1. Chat: User — `ChatRequest` — `Client` — `Provider` — HTTP — LLM — `ChatResponse`
2. Agent: User — `Runner.Run()` — loop(Agent — LLM — ToolCalls — Execute — Results) — text response or max turns
3. RAG: User — `QueryRequest` — `Retriever` — HTTP — Backend — `QueryResponse` — `FormatContext()` — system prompt — Chat

## Dependencies

- **nlohmann/json** (`third_party/nlohmann/`): Vendored. Exception-safe via `parse(raw, nullptr, false)`.
- **GoogleTest** v1.17.0: FetchContent, PC only (`BUILD_TESTING=ON`)
- **CPR** v1.9.9: FetchContent, PC only. Last C++14-compatible version. Builds own curl.
- **ArduinoHttpClient** >=0.6.1: PlatformIO, Arduino only (declared in `library.json`)

## Build & Test

```bash
# PC build + test
mkdir -p build && cd build && cmake -DBUILD_TESTING=ON .. && make -j4
ctest --output-on-failure

# Embedded (PlatformIO)
pio run -d pio/build_test -e esp32dev                          # Full build
pio run -d examples/embedded/{foresthub,openai,gemini,anthropic}/chat -e esp32dev

# Format & Lint
find include src examples tests -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
clang-tidy src/**/*.cpp -- -std=c++14 -I./include -I./third_party
```

**Running apps** (from build dir): `FORESTHUB_API_KEY=... ./foresthub_chat`, `OPENAI_API_KEY=... ./foresthub_chat_openai`, `GEMINI_API_KEY=... ./foresthub_chat_gemini`, `ANTHROPIC_API_KEY=... ./foresthub_chat_anthropic`

## Testing

422+ tests, 8 executables. Hand-rolled mocks (no GMock — `-fno-rtti` incompatible).

| Executable | Tests | Scope |
|------------|-------|-------|
| `run_core_tests` | 101 | Input, model, options, tools, types, json, client |
| `run_agent_tests` | 42 | Agent construction/fluent API, Runner execution, options |
| `run_provider_tests` | ~149 | ForestHub + OpenAI + Gemini + Anthropic HTTP/retry/error/schema via MockHttpClient |
| `run_platform_tests` | 44 | PC CreatePlatform factory, subsystems, GPIO, ENABLE macros, console |
| `run_rag_tests` | 15 | RemoteRetriever HTTP/retry/JSON/serialization/FormatContext via MockHttpClient |
| `run_integration_tests` | 7 | Runner-Client-Provider chain, multi-provider routing |
| `run_contract_tests` | 12 | ForestHub API JSON schema verification |
| `run_util_tests` | 50 | Optional polyfill, Ticker, Schema normalization |

Mock infrastructure: `tests/mocks/` (MockHttpClient, MockProvider, MockLLMClient). Response queue pattern for `Chat()`.

## Configuration Pattern

Providers share `ProviderConfig` struct. Grouped in `ClientConfig.remote`:
```cpp
foresthub::config::ProviderConfig cfg;
cfg.api_key = std::getenv("OPENAI_API_KEY");
cfg.supported_models = {"gpt-4o", "gpt-4o-mini"};
client_cfg.remote.openai = cfg;  // .foresthub, .gemini, .anthropic analogous

auto client = foresthub::Client::Create(client_cfg, http_client);
```

## Tool Creation Pattern

```cpp
struct MyArgs { std::string query; int limit; };
void from_json(const json& j, MyArgs& a) {
    a.query = j.value("query", "");  // .value() with defaults — no throws
    a.limit = j.value("limit", 10);
}
json handler(const MyArgs& a) { return json{{"result", a.query}}; }

auto tool = foresthub::core::NewFunctionTool<MyArgs, json>("my_tool", "Description", schema, handler);
// Raw JSON pass-through: NewFunctionTool<json, json>(...)
```

## License

This project is licensed under the [GNU Affero General Public License v3.0](https://github.com/ForestHubAI/fh-sdk/blob/main/LICENSE) with a commercial licensing option. See [LICENSE](https://github.com/ForestHubAI/fh-sdk/blob/main/LICENSE) for details.
