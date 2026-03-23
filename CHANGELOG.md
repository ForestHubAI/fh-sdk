# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.2] - 2026-03-23

### Added

- `Ticker::WithTimezoneOffset()` builder and `SetTimezoneOffset()` for timezone-aware calendar scheduling (Daily/Weekly/Hourly) -- pass raw UTC epoch to Start()/Check(), Ticker applies offset internally
- `Ticker::tz_offset_sec()` getter for reading configured timezone offset
- `TimeInterface::GetLocalEpoch()` non-virtual convenience method returning `GetEpochTime() + utc_offset_sec()`

### Changed

- `TimeConfig.gmt_offset_sec` renamed to `std_offset_sec` -- industry-standard "standard offset" terminology (Java, NodaTime, IANA)
- `TimeConfig.daylight_offset_sec` renamed to `dst_offset_sec` -- universally understood "DST" terminology
- `SetOffset()` parameter names updated accordingly (`std_offset_sec`, `dst_offset_sec`)

### Fixed

- `GetLocalTime()` now sets `tm_isdst` field based on whether DST offset is active
- Ticker documentation references correct method name `utc_offset_sec()` and shows `WithTimezoneOffset()` pattern

## [0.1.1] - 2026-03-16

### Added

- `foresthub::util::StrPrintf` -- printf-style string formatting with single heap allocation via two-pass vsnprintf, preventing heap fragmentation on memory-constrained embedded devices (ESP32) for large RAG contexts

## [0.1.0] - 2026-03-12

### Added

- Multi-provider LLM support with native API implementations for ForestHub, OpenAI (Responses API), Google Gemini (generateContent API), and Anthropic Claude (Messages API)
- Automatic request routing to the correct provider based on model ID
- Agent framework with tool calling, multi-turn conversations, agent handoffs, and generation parameters (temperature, max_tokens, top_p)
- Tool system: `ExternalTool` (user-defined schema), `FunctionTool` (C++ callback with typed args), `InternalToolCall` (provider-managed, e.g. web search)
- RAG retriever with remote backend query (`POST /rag/query`) and XML context formatting for LLM prompt injection
- Hardware Abstraction Layer (HAL) for PC (Linux/macOS) and Arduino (ESP32, Portenta H7)
- Optional HAL subsystems via compile-time macros: `FORESTHUB_ENABLE_NETWORK`, `FORESTHUB_ENABLE_CRYPTO`, `FORESTHUB_ENABLE_GPIO`
- Unified Ticker utility with Periodic, OneShot, Daily, Weekly, and Hourly scheduling modes
- Non-blocking console read (`TryReadLine`) with poll-based PC and non-blocking Arduino implementations
- Composable JSON Schema normalization with provider-specific strictification pipelines
- Fluent builder API for `ChatRequest`, `Agent`, and configuration structs
- `foresthub::Optional<T>` polyfill for C++14 environments without `<optional>`
- PlatformIO library support (`lib_deps = ForestHubAI/fh-sdk`)
- Comprehensive test suite (422 tests across 8 executables)
- Doxygen API reference generation
- C++14 compatible, no exceptions, no RTTI -- targets any C++14-capable toolchain

[0.1.1]: https://github.com/ForestHubAI/fh-sdk/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/ForestHubAI/fh-sdk/releases/tag/v0.1.0
