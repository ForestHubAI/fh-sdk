# Provider Guide

The ForestHub SDK supports four LLM providers through a unified interface. Each provider uses the same `ProviderConfig` struct and the same `Client::Chat()` method. The `Client` routes requests to the correct provider based on the model ID.

## Overview

All providers share a common configuration struct:

```cpp
foresthub::config::ProviderConfig cfg;
cfg.api_key = "your-api-key";                   // Authentication token
cfg.base_url = "";                               // Empty = provider default
cfg.supported_models = {"model-a", "model-b"};  // Models routed to this provider
```

Providers are grouped in `RemoteProviders` and set on `ClientConfig.remote`:

```cpp
foresthub::config::ClientConfig client_cfg;
client_cfg.remote.foresthub = fh_cfg;
client_cfg.remote.openai = oai_cfg;
client_cfg.remote.gemini = gemini_cfg;
client_cfg.remote.anthropic = anth_cfg;
```

Only providers with a config value are created. The `Client` is then built with:

```cpp
std::unique_ptr<foresthub::Client> client = foresthub::Client::Create(client_cfg, http_client);
```

## ForestHub

ForestHub is a backend service that proxies requests to various LLM providers. It is the default provider for embedded deployments.

### Configuration

```cpp
foresthub::config::ProviderConfig fh_cfg;
fh_cfg.api_key = std::getenv("FORESTHUB_API_KEY");
fh_cfg.base_url = "https://fh-backend-368736749905.europe-west1.run.app";
fh_cfg.supported_models = {"gpt-4.1", "gpt-4.1-mini", "gpt-4o", "gpt-4o-mini"};

foresthub::config::ClientConfig cfg;
cfg.remote.foresthub = fh_cfg;
```

| Property | Value |
|----------|-------|
| Environment variable | `FORESTHUB_API_KEY` |
| Default base URL | (none -- must be set explicitly) |
| Authentication | `Device-Key` header |
| Supported features | Chat, tool calling, web search, structured output, RAG |

> See [`examples/pc/foresthub/`](https://github.com/ForestHubAI/fh-sdk/tree/main/examples/pc/foresthub) for complete examples.

## OpenAI

Direct access to OpenAI using the native Responses API.

### Configuration

```cpp
foresthub::config::ProviderConfig oai_cfg;
oai_cfg.api_key = std::getenv("OPENAI_API_KEY");
oai_cfg.supported_models = {"gpt-4.1", "gpt-4.1-mini", "gpt-4o", "gpt-4o-mini"};

foresthub::config::ClientConfig cfg;
cfg.remote.openai = oai_cfg;
```

| Property | Value |
|----------|-------|
| Environment variable | `OPENAI_API_KEY` |
| Default base URL | `https://api.openai.com` |
| Authentication | `Authorization: Bearer` header |
| Supported features | Chat, tool calling, web search, structured output |

> See [`examples/pc/openai/`](https://github.com/ForestHubAI/fh-sdk/tree/main/examples/pc/openai) for complete examples.

## Google Gemini

Direct access to Google Gemini using the native generateContent API.

### Configuration

```cpp
foresthub::config::ProviderConfig gemini_cfg;
gemini_cfg.api_key = std::getenv("GEMINI_API_KEY");
gemini_cfg.supported_models = {"gemini-2.5-pro", "gemini-2.5-flash", "gemini-2.5-flash-lite"};

foresthub::config::ClientConfig cfg;
cfg.remote.gemini = gemini_cfg;
```

| Property | Value |
|----------|-------|
| Environment variable | `GEMINI_API_KEY` |
| Default base URL | `https://generativelanguage.googleapis.com` |
| Authentication | `x-goog-api-key` header |
| Supported features | Chat, tool calling, web search, structured output |

### Notes

- The SDK uses the `v1beta` API endpoint, which is required for function calling support.
- Function calling and web search cannot be combined in the same request. If you need both, use separate requests.

> See [`examples/pc/gemini/`](https://github.com/ForestHubAI/fh-sdk/tree/main/examples/pc/gemini) for complete examples.

## Anthropic Claude

Direct access to Anthropic Claude using the native Messages API.

### Configuration

```cpp
foresthub::config::ProviderConfig anthropic_cfg;
anthropic_cfg.api_key = std::getenv("ANTHROPIC_API_KEY");
anthropic_cfg.supported_models = {"claude-sonnet-4-6", "claude-haiku-4-5", "claude-opus-4-6"};

foresthub::config::ClientConfig cfg;
cfg.remote.anthropic = anthropic_cfg;
```

| Property | Value |
|----------|-------|
| Environment variable | `ANTHROPIC_API_KEY` |
| Default base URL | `https://api.anthropic.com` |
| Authentication | `x-api-key` header + `anthropic-version: 2023-06-01` |
| Supported features | Chat, tool calling, structured output |

### Limitations

- **Web search is not supported.** Anthropic's server-side web search tool returns encrypted content that cannot be processed in multi-turn conversations. Use a different provider for web search.

> See [`examples/pc/anthropic/`](https://github.com/ForestHubAI/fh-sdk/tree/main/examples/pc/anthropic) for complete examples.

## Multi-Provider Routing

When multiple providers are configured, the `Client` routes each request to the provider whose `supported_models` list contains the requested model ID:

```cpp
foresthub::config::ClientConfig cfg;

foresthub::config::ProviderConfig oai_cfg;
oai_cfg.api_key = std::getenv("OPENAI_API_KEY");
oai_cfg.supported_models = {"gpt-4.1", "gpt-4.1-mini"};
cfg.remote.openai = oai_cfg;

foresthub::config::ProviderConfig gemini_cfg;
gemini_cfg.api_key = std::getenv("GEMINI_API_KEY");
gemini_cfg.supported_models = {"gemini-2.5-flash"};
cfg.remote.gemini = gemini_cfg;

auto client = foresthub::Client::Create(cfg, http_client);

// Routes to OpenAI provider
foresthub::core::ChatRequest req_oai;
req_oai.model = "gpt-4.1-mini";
req_oai.input = std::make_shared<foresthub::core::InputString>("Hello");
client->Chat(req_oai);

// Routes to Gemini provider
foresthub::core::ChatRequest req_gemini;
req_gemini.model = "gemini-2.5-flash";
req_gemini.input = std::make_shared<foresthub::core::InputString>("Hello");
client->Chat(req_gemini);
```

Use `client->SupportsModel("model-id")` to check if any registered provider handles a given model before sending a request.

## Custom Base URLs

All providers accept a custom `base_url` for self-hosted endpoints or proxies:

```cpp
foresthub::config::ProviderConfig cfg;
cfg.api_key = "your-key";
cfg.base_url = "https://my-proxy.example.com";
cfg.supported_models = {"gpt-4.1"};
```

When `base_url` is empty, each provider falls back to its built-in default URL.
