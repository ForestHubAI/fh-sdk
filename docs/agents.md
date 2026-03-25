# Agent Framework

The agent framework provides a structured way to build LLM-powered workflows with tool calling, multi-turn execution, and agent handoffs.

## Overview

The framework has two core components:

- **Agent** -- holds instructions (system prompt), tools, and generation options.
- **Runner** -- executes the agent loop: send a request to the LLM, dispatch any tool calls, feed results back, and repeat until the LLM produces a final text response or the turn limit is reached.

## Creating an Agent

Use the fluent builder API to configure an agent:

```cpp
#include "foresthub/agent/agent.hpp"
#include "foresthub/llm/options.hpp"

auto agent = std::make_shared<foresthub::agent::Agent>("MyAssistant");
agent->WithInstructions("You are a helpful assistant. Use tools when appropriate.")
     .WithOptions(foresthub::core::Options().WithTemperature(0.7f).WithMaxTokens(2048))
     .AddTool(my_tool);
```

| Method | Purpose |
|--------|---------|
| `WithInstructions(str)` | Set the system prompt sent to the LLM |
| `WithOptions(opts)` | Set generation parameters (temperature, max_tokens, etc.) |
| `WithTools(vec)` | Replace all tools with the given list |
| `AddTool(tool)` | Append a single tool |
| `WithResponseFormat(fmt)` | Set structured output format (JSON schema) |

## Tools

The SDK supports three tool types:

### FunctionTool

A C++ callback that the LLM can invoke. This is the most common tool type.

The pattern is: define an argument struct, write a `from_json` function, write a handler, and use `NewFunctionTool` to wire them together.

```cpp
#include "foresthub/llm/tools.hpp"
#include "foresthub/util/json.hpp"

using json = nlohmann::json;

// 1. Argument struct
struct LookupArgs {
    std::string query;
    int limit;
};

// 2. JSON deserialization (use j.value() with defaults -- never j.at())
void from_json(const json& j, LookupArgs& a) {
    a.query = j.value("query", "");
    a.limit = j.value("limit", 10);
}

// 3. Handler function
json DoLookup(const LookupArgs& a) {
    return json{{"results", a.query}, {"count", a.limit}};
}

// 4. Create the tool
json schema = json::parse(R"({
    "type": "object",
    "properties": {
        "query": {"type": "string", "description": "Search query"},
        "limit": {"type": "integer", "description": "Max results"}
    },
    "required": ["query"]
})", nullptr, false);

auto tool = foresthub::core::NewFunctionTool<LookupArgs, json>(
    "lookup",             // Tool name visible to the LLM
    "Search for items.",  // Description sent to the LLM
    schema,               // JSON schema for parameters
    DoLookup              // C++ handler
);
```

For raw JSON pass-through (when you do not need a typed struct), use `NewFunctionTool<json, json>(...)`.

### ExternalTool

A tool defined with a name, description, and parameter schema but without a C++ callback. When the LLM requests an external tool call, the `Runner` returns the call in `ChatResponse::tool_call_requests` for you to handle outside the agent loop. Use `ExternalToolBase` to define one:

```cpp
auto ext_tool = std::make_shared<foresthub::core::ExternalToolBase>();
ext_tool->name = "send_email";
ext_tool->description = "Sends an email to the given address.";
ext_tool->parameters = schema;
```

### InternalToolCall (WebSearch)

A built-in tool managed by the LLM provider on the server side. The SDK includes `WebSearch`:

```cpp
auto web_search = std::make_shared<foresthub::core::WebSearch>();
agent->AddTool(web_search);
```

When the LLM uses web search, the query is logged in `ChatResponse::tools_called`. No C++ callback is needed -- the provider handles execution.

> **Note:** Web search is not supported by the Anthropic provider. On Gemini, web search cannot be combined with function calling in the same request.

## Running an Agent

The `Runner` manages the execution loop:

```cpp
#include "foresthub/agent/runner.hpp"

// Create a runner with the LLM client and default model
auto runner = std::make_shared<foresthub::agent::Runner>(client, "gpt-4.1-mini");
runner->WithMaxTurns(5);

// Run the agent
auto input = std::make_shared<foresthub::core::InputString>("What is the weather in Paris?");
foresthub::agent::RunResultOrError result = runner->Run(agent, input);

if (result.HasError()) {
    printf("Error: %s\n", result.error.c_str());
} else {
    const json& output = result.result->final_output;
    if (output.is_string()) {
        printf("Agent: %s\n", output.get<std::string>().c_str());
    } else {
        printf("Agent: %s\n", output.dump().c_str());
    }
    printf("Turns used: %d\n", result.result->turns);
}
```

### RunResult

On success, `RunResultOrError::result` contains:

| Field | Type | Description |
|-------|------|-------------|
| `final_output` | `json` | The agent's final response (text or structured JSON) |
| `last_agent` | `std::shared_ptr<Agent>` | The agent that was active when execution finished |
| `turns` | `int` | Number of LLM round-trips taken |

### Error Handling

Check `result.HasError()` before accessing `result.result`. The `error` string describes what went wrong (for example, no provider supports the model, or the LLM returned an invalid response).

## Generation Options

Control LLM behavior with the `Options` struct:

```cpp
foresthub::core::Options opts;
opts.WithTemperature(0.5f)       // Randomness (0.0 to 1.0)
    .WithMaxTokens(2048)         // Maximum tokens to generate
    .WithTopK(40)                // Top-k sampling limit
    .WithTopP(0.9f)              // Nucleus sampling cutoff
    .WithFrequencyPenalty(0.5f)  // Reduces repetition
    .WithPresencePenalty(0.3f)   // Encourages new concepts
    .WithSeed(42);               // Deterministic output

agent->WithOptions(opts);
```

Options can also be set directly on a `ChatRequest` using the same fluent methods (e.g., `req.WithTemperature(0.5f)`).

## Structured Output

Force the LLM to return valid JSON conforming to a schema:

```cpp
foresthub::core::ResponseFormat fmt;
fmt.name = "weather_response";
fmt.description = "Structured weather data.";
fmt.schema = json::parse(R"({
    "type": "object",
    "properties": {
        "city": {"type": "string"},
        "temperature": {"type": "number"},
        "unit": {"type": "string", "enum": ["celsius", "fahrenheit"]}
    },
    "required": ["city", "temperature", "unit"]
})", nullptr, false);

agent->WithResponseFormat(fmt);
```

The SDK automatically normalizes the schema to meet each provider's strictification requirements.

## Agent Handoffs

An agent can be exposed as a tool so that another agent can delegate work to it:

```cpp
auto researcher = std::make_shared<foresthub::agent::Agent>("Researcher");
researcher->WithInstructions("You research topics thoroughly.");

auto coordinator = std::make_shared<foresthub::agent::Agent>("Coordinator");
coordinator->WithInstructions("You coordinate tasks. Delegate research to the researcher tool.")
           .AddTool(researcher->AsTool("research", "Delegates a research query.", runner));
```

When the coordinator calls the `research` tool, the `Runner` executes the researcher agent and returns its output as the tool result.

## Max Turns

Limit the number of LLM round-trips to prevent infinite loops:

```cpp
runner->WithMaxTurns(10);
```

If `WithMaxTurns` is not called, the runner has no turn limit. Each tool call and response counts as one turn.

## Further Reading

- [Getting Started](https://github.com/ForestHubAI/fh-sdk/blob/main/docs/getting-started.md) -- build the SDK and run your first example
- [Provider Guide](https://github.com/ForestHubAI/fh-sdk/blob/main/docs/providers.md) -- configure each LLM provider
- [`examples/pc/foresthub/agent.cpp`](https://github.com/ForestHubAI/fh-sdk/blob/main/examples/pc/foresthub/agent.cpp) -- complete agent example
