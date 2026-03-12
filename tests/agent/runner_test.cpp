// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "foresthub/agent/runner.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "foresthub/agent/agent.hpp"
#include "foresthub/agent/handoff.hpp"
#include "foresthub/core/input.hpp"
#include "foresthub/core/options.hpp"
#include "foresthub/core/tools.hpp"
#include "foresthub/util/json.hpp"
#include "mocks/mock_llm_client.hpp"

using namespace foresthub::agent;
using namespace foresthub::core;
using json = nlohmann::json;

// Helper: create a simple FunctionTool.
struct EchoArgs {
    std::string text;
};

// ADL hook required by nlohmann::json — must be snake_case.
void from_json(const json& j, EchoArgs& args) {
    args.text = j.value("text", "");
}

// Mock ExternalTool with kWebSearch type — triggers "unsupported tool execution type" in Runner.
class UnsupportedTypeTool : public ExternalToolBase {
public:
    UnsupportedTypeTool() {
        name = "unsupported_tool";
        description = "Tool with unsupported execution type";
        parameters = json::object();
    }
    ToolType GetToolType() const override { return ToolType::kWebSearch; }
};

static std::shared_ptr<FunctionTool> MakeEchoTool() {
    json schema = {{"type", "object"}, {"properties", {{"text", {{"type", "string"}}}}}};
    std::function<json(EchoArgs)> handler = [](const EchoArgs& args) -> json { return json("echo: " + args.text); };
    return NewFunctionTool<EchoArgs, json>("echo", "Echoes text", schema, handler);
}

// Helper: create a ChatResponse with text only.
static std::shared_ptr<ChatResponse> TextResponse(const std::string& text) {
    auto resp = std::make_shared<ChatResponse>();
    resp->text = text;
    return resp;
}

// Helper: create a ChatResponse with a tool call request.
static std::shared_ptr<ChatResponse> ToolCallResponse(const std::string& tool_name, const std::string& call_id,
                                                      const std::string& args) {
    auto resp = std::make_shared<ChatResponse>();
    ToolCallRequest tcr;
    tcr.call_id = call_id;
    tcr.name = tool_name;
    tcr.arguments = args;
    resp->tool_call_requests.push_back(tcr);
    return resp;
}

// ==========================================================================
// 1. RunResultOrError
// ==========================================================================

TEST(RunResultOrErrorTest, Success) {
    RunResult run_result;
    run_result.final_output = json("hello");
    run_result.turns = 1;
    RunResultOrError result = RunResultOrError::Success(std::move(run_result));
    EXPECT_FALSE(result.HasError());
    EXPECT_TRUE(result.result.HasValue());
    EXPECT_EQ(result.result->final_output, json("hello"));
}

TEST(RunResultOrErrorTest, Failure) {
    RunResultOrError result = RunResultOrError::Failure("something went wrong");
    EXPECT_TRUE(result.HasError());
    EXPECT_EQ(result.error, "something went wrong");
}

// ==========================================================================
// 2. Runner Construction
// ==========================================================================

TEST(RunnerTest, Construction) {
    auto mock = std::make_shared<foresthub::tests::MockLLMClient>();
    auto runner = std::make_shared<Runner>(mock, "gpt-4o");
    EXPECT_EQ(runner->default_model(), "gpt-4o");
    EXPECT_FALSE(runner->max_turns().HasValue());
}

TEST(RunnerTest, WithMaxTurns) {
    auto mock = std::make_shared<foresthub::tests::MockLLMClient>();
    auto runner = std::make_shared<Runner>(mock, "gpt-4o");
    Runner& ref = runner->WithMaxTurns(5);
    EXPECT_TRUE(runner->max_turns().HasValue());
    EXPECT_EQ(*runner->max_turns(), 5);
    EXPECT_EQ(&ref, runner.get());
}

// ==========================================================================
// 3. Runner::Run — Error cases
// ==========================================================================

TEST(RunnerTest, NullAgent) {
    auto mock = std::make_shared<foresthub::tests::MockLLMClient>();
    auto runner = std::make_shared<Runner>(mock, "gpt-4o");
    auto input = std::make_shared<InputString>("hello");

    RunResultOrError result = runner->Run(nullptr, input);
    EXPECT_TRUE(result.HasError());
    EXPECT_EQ(result.error, "starting_agent cannot be null");
}

TEST(RunnerTest, NetworkError) {
    auto mock = std::make_shared<foresthub::tests::MockLLMClient>();
    // No responses queued — Chat() returns nullptr.
    auto runner = std::make_shared<Runner>(mock, "gpt-4o");

    auto agent = std::make_shared<Agent>("a");
    auto input = std::make_shared<InputString>("hello");

    RunResultOrError result = runner->Run(agent, input);
    EXPECT_TRUE(result.HasError());
    EXPECT_TRUE(result.error.find("Network error") != std::string::npos);
}

TEST(RunnerTest, NoTextNoTools) {
    auto mock = std::make_shared<foresthub::tests::MockLLMClient>();
    // Response with empty text and empty tool_call_requests.
    mock->responses.push_back(std::make_shared<ChatResponse>());

    auto runner = std::make_shared<Runner>(mock, "gpt-4o");
    auto agent = std::make_shared<Agent>("a");
    auto input = std::make_shared<InputString>("hello");

    RunResultOrError result = runner->Run(agent, input);
    EXPECT_TRUE(result.HasError());
    EXPECT_TRUE(result.error.find("produced neither final output nor tool calls") != std::string::npos);
}

// ==========================================================================
// 4. Runner::Run — Single-turn
// ==========================================================================

TEST(RunnerTest, SingleTurnTextResponse) {
    auto mock = std::make_shared<foresthub::tests::MockLLMClient>();
    mock->responses.push_back(TextResponse("Hello, world!"));

    auto runner = std::make_shared<Runner>(mock, "gpt-4o");
    auto agent = std::make_shared<Agent>("a");
    auto input = std::make_shared<InputString>("hi");

    RunResultOrError result = runner->Run(agent, input);
    ASSERT_FALSE(result.HasError());
    EXPECT_EQ(result.result->final_output, json("Hello, world!"));
    EXPECT_EQ(result.result->turns, 1);
    EXPECT_EQ(result.result->last_agent->name(), "a");
}

TEST(RunnerTest, SingleTurnWithResponseFormat) {
    auto mock = std::make_shared<foresthub::tests::MockLLMClient>();
    // Response has text AND tool calls — but response_format is set, so text wins.
    auto resp = std::make_shared<ChatResponse>();
    resp->text = "formatted output";
    ToolCallRequest tcr;
    tcr.call_id = "c1";
    tcr.name = "echo";
    tcr.arguments = R"({"text":"ignored"})";
    resp->tool_call_requests.push_back(tcr);
    mock->responses.push_back(resp);

    auto runner = std::make_shared<Runner>(mock, "gpt-4o");
    auto agent = std::make_shared<Agent>("a");
    ResponseFormat fmt;
    fmt.name = "json_output";
    agent->WithResponseFormat(fmt);
    auto input = std::make_shared<InputString>("hi");

    RunResultOrError result = runner->Run(agent, input);
    ASSERT_FALSE(result.HasError());
    EXPECT_EQ(result.result->final_output, json("formatted output"));
    EXPECT_EQ(result.result->turns, 1);
}

// ==========================================================================
// 5. Runner::Run — Multi-turn tool calling
// ==========================================================================

TEST(RunnerTest, MultiTurnToolCalling) {
    auto mock = std::make_shared<foresthub::tests::MockLLMClient>();
    // Turn 1: LLM requests tool call.
    mock->responses.push_back(ToolCallResponse("echo", "c1", R"({"text":"hello"})"));
    // Turn 2: LLM returns text (final output).
    mock->responses.push_back(TextResponse("Done"));

    auto runner = std::make_shared<Runner>(mock, "gpt-4o");
    auto agent = std::make_shared<Agent>("a");
    agent->AddTool(MakeEchoTool());
    auto input = std::make_shared<InputString>("do something");

    RunResultOrError result = runner->Run(agent, input);
    ASSERT_FALSE(result.HasError());
    EXPECT_EQ(result.result->final_output, json("Done"));
    EXPECT_EQ(result.result->turns, 2);
    EXPECT_EQ(mock->call_count, 2);
}

TEST(RunnerTest, ToolNotFound) {
    auto mock = std::make_shared<foresthub::tests::MockLLMClient>();
    // LLM requests a tool that the agent doesn't have.
    mock->responses.push_back(ToolCallResponse("nonexistent", "c1", "{}"));

    auto runner = std::make_shared<Runner>(mock, "gpt-4o");
    auto agent = std::make_shared<Agent>("a");
    auto input = std::make_shared<InputString>("hi");

    RunResultOrError result = runner->Run(agent, input);
    EXPECT_TRUE(result.HasError());
    EXPECT_TRUE(result.error.find("Tool nonexistent not found") != std::string::npos);
}

TEST(RunnerTest, UnsupportedToolType) {
    auto mock = std::make_shared<foresthub::tests::MockLLMClient>();
    // WebSearch is not external (IsExternal()=false), so FindExternalTool returns nullptr
    // and the runner hits the "Tool not found" error path.
    mock->responses.push_back(ToolCallResponse("web_search", "c1", "{}"));

    auto runner = std::make_shared<Runner>(mock, "gpt-4o");
    auto agent = std::make_shared<Agent>("a");
    agent->AddTool(std::make_shared<WebSearch>());
    auto input = std::make_shared<InputString>("hi");

    RunResultOrError result = runner->Run(agent, input);
    EXPECT_TRUE(result.HasError());
    // WebSearch is not external, so FindExternalTool returns nullptr → "not found".
    EXPECT_TRUE(result.error.find("Tool web_search not found") != std::string::npos);
}

// ==========================================================================
// 6. Runner::Run — Max turns
// ==========================================================================

TEST(RunnerTest, MaxTurnsExceeded) {
    auto mock = std::make_shared<foresthub::tests::MockLLMClient>();
    // Always return tool calls — never a final text.
    mock->responses.push_back(ToolCallResponse("echo", "c1", R"({"text":"1"})"));
    mock->responses.push_back(ToolCallResponse("echo", "c2", R"({"text":"2"})"));

    auto runner = std::make_shared<Runner>(mock, "gpt-4o");
    runner->WithMaxTurns(1);
    auto agent = std::make_shared<Agent>("a");
    agent->AddTool(MakeEchoTool());
    auto input = std::make_shared<InputString>("hi");

    RunResultOrError result = runner->Run(agent, input);
    EXPECT_TRUE(result.HasError());
    EXPECT_EQ(result.error, "Max turns exceeded");
}

TEST(RunnerTest, MaxTurnsUnlimited) {
    auto mock = std::make_shared<foresthub::tests::MockLLMClient>();
    // 3 turns of tool calls, then text.
    mock->responses.push_back(ToolCallResponse("echo", "c1", R"({"text":"1"})"));
    mock->responses.push_back(ToolCallResponse("echo", "c2", R"({"text":"2"})"));
    mock->responses.push_back(ToolCallResponse("echo", "c3", R"({"text":"3"})"));
    mock->responses.push_back(TextResponse("Final"));

    auto runner = std::make_shared<Runner>(mock, "gpt-4o");
    // No WithMaxTurns — unlimited.
    auto agent = std::make_shared<Agent>("a");
    agent->AddTool(MakeEchoTool());
    auto input = std::make_shared<InputString>("hi");

    RunResultOrError result = runner->Run(agent, input);
    ASSERT_FALSE(result.HasError());
    EXPECT_EQ(result.result->final_output, json("Final"));
    EXPECT_EQ(result.result->turns, 4);
}

// ==========================================================================
// 7. Runner::Run — Handoff
// ==========================================================================

TEST(RunnerTest, HandoffSwitchesAgent) {
    auto mock = std::make_shared<foresthub::tests::MockLLMClient>();

    auto agent_b = std::make_shared<Agent>("agent-b");
    agent_b->WithInstructions("I am B");

    auto agent_a = std::make_shared<Agent>("agent-a");
    auto handoff = NewHandoff("transfer_to_b", "Transfer to B", agent_b);
    agent_a->AddTool(handoff);

    // Turn 1: agent-a invokes handoff.
    mock->responses.push_back(ToolCallResponse("transfer_to_b", "c1", "{}"));
    // Turn 2: agent-b returns text.
    mock->responses.push_back(TextResponse("Hello from B"));

    auto runner = std::make_shared<Runner>(mock, "gpt-4o");
    auto input = std::make_shared<InputString>("hi");

    RunResultOrError result = runner->Run(agent_a, input);
    ASSERT_FALSE(result.HasError());
    EXPECT_EQ(result.result->last_agent->name(), "agent-b");
    EXPECT_EQ(result.result->final_output, json("Hello from B"));
    // Turn 1 (handoff) + Turn 2 (text from B).
    EXPECT_EQ(result.result->turns, 2);
}

TEST(RunnerTest, HandoffWithModelOverride) {
    auto mock = std::make_shared<foresthub::tests::MockLLMClient>();

    auto agent_b = std::make_shared<Agent>("agent-b");
    auto agent_a = std::make_shared<Agent>("agent-a");
    auto handoff = NewHandoff("transfer_to_b", "Transfer to B", agent_b, "gpt-4o-mini");
    agent_a->AddTool(handoff);

    // Turn 1: handoff.
    mock->responses.push_back(ToolCallResponse("transfer_to_b", "c1", "{}"));
    // Turn 2: text from B.
    mock->responses.push_back(TextResponse("B response"));

    auto runner = std::make_shared<Runner>(mock, "gpt-4o");
    auto input = std::make_shared<InputString>("hi");

    RunResultOrError result = runner->Run(agent_a, input);
    ASSERT_FALSE(result.HasError());

    ASSERT_EQ(mock->captured_requests.size(), 2u);
    EXPECT_EQ(mock->captured_requests[0].model, "gpt-4o");
    EXPECT_EQ(mock->captured_requests[1].model, "gpt-4o-mini");
}

TEST(RunnerTest, ToolResultFeedback) {
    auto mock = std::make_shared<foresthub::tests::MockLLMClient>();
    // Turn 1: tool call.
    mock->responses.push_back(ToolCallResponse("echo", "c1", R"({"text":"hello"})"));
    // Turn 2: final text.
    mock->responses.push_back(TextResponse("Done"));

    auto runner = std::make_shared<Runner>(mock, "gpt-4o");
    auto agent = std::make_shared<Agent>("a");
    agent->AddTool(MakeEchoTool());
    auto input = std::make_shared<InputString>("hi");

    RunResultOrError result = runner->Run(agent, input);
    ASSERT_FALSE(result.HasError());

    ASSERT_EQ(mock->captured_requests.size(), 2u);
    std::shared_ptr<Input> second_input = mock->captured_requests[1].input;
    ASSERT_NE(second_input, nullptr);
    EXPECT_EQ(second_input->GetInputType(), InputType::kItems);
    auto items = std::static_pointer_cast<InputItems>(second_input);
    // Original input (1) + ToolCallRequest (1) + ToolResult (1) = 3 items.
    EXPECT_EQ(items->items.size(), 3u);
}

TEST(RunnerTest, TextAndToolCallsContinuesExecution) {
    // When response has BOTH text AND tool calls but NO response_format,
    // the runner should ignore the text and execute the tools.
    auto mock = std::make_shared<foresthub::tests::MockLLMClient>();

    // Turn 1: text + tool calls → text ignored, tools executed.
    auto resp = std::make_shared<ChatResponse>();
    resp->text = "partial thinking";
    ToolCallRequest tcr;
    tcr.call_id = "c1";
    tcr.name = "echo";
    tcr.arguments = R"({"text":"hello"})";
    resp->tool_call_requests.push_back(tcr);
    mock->responses.push_back(resp);

    // Turn 2: final text only.
    mock->responses.push_back(TextResponse("Done"));

    auto runner = std::make_shared<Runner>(mock, "gpt-4o");
    auto agent = std::make_shared<Agent>("a");
    agent->AddTool(MakeEchoTool());
    auto input = std::make_shared<InputString>("hi");

    RunResultOrError result = runner->Run(agent, input);
    ASSERT_FALSE(result.HasError());
    EXPECT_EQ(result.result->final_output, json("Done"));
    EXPECT_EQ(result.result->turns, 2);
}

// ==========================================================================
// 8. Runner::Run — Multiple tool calls in single response
// ==========================================================================

TEST(RunnerTest, MultipleToolCallsInSingleResponse) {
    auto mock = std::make_shared<foresthub::tests::MockLLMClient>();

    // Turn 1: LLM requests 2 tool calls in one response.
    auto resp = std::make_shared<ChatResponse>();
    ToolCallRequest tcr1;
    tcr1.call_id = "c1";
    tcr1.name = "echo";
    tcr1.arguments = R"({"text":"one"})";
    resp->tool_call_requests.push_back(tcr1);
    ToolCallRequest tcr2;
    tcr2.call_id = "c2";
    tcr2.name = "echo";
    tcr2.arguments = R"({"text":"two"})";
    resp->tool_call_requests.push_back(tcr2);
    mock->responses.push_back(resp);

    // Turn 2: LLM returns final text.
    mock->responses.push_back(TextResponse("Done"));

    auto runner = std::make_shared<Runner>(mock, "gpt-4o");
    auto agent = std::make_shared<Agent>("a");
    agent->AddTool(MakeEchoTool());
    auto input = std::make_shared<InputString>("hi");

    RunResultOrError result = runner->Run(agent, input);
    ASSERT_FALSE(result.HasError());
    EXPECT_EQ(result.result->final_output, json("Done"));
    EXPECT_EQ(result.result->turns, 2);
    EXPECT_EQ(mock->call_count, 2);

    // Verify history: original input + 2xToolCallRequest + 2xToolResult = 5 items.
    ASSERT_EQ(mock->captured_requests.size(), 2u);
    std::shared_ptr<Input> second_input = mock->captured_requests[1].input;
    ASSERT_NE(second_input, nullptr);
    EXPECT_EQ(second_input->GetInputType(), InputType::kItems);
    auto items = std::static_pointer_cast<InputItems>(second_input);
    EXPECT_EQ(items->items.size(), 5u);
}

// ==========================================================================
// 9. Runner::Run — Null-handoff edge case
// ==========================================================================

TEST(RunnerTest, HandoffWithNullTargetAgent) {
    auto mock = std::make_shared<foresthub::tests::MockLLMClient>();

    auto agent = std::make_shared<Agent>("a");
    // Create a handoff with nullptr target.
    auto handoff = NewHandoff("transfer_to_null", "Transfer to null", nullptr);
    agent->AddTool(handoff);

    // Turn 1: invoke the null handoff.
    mock->responses.push_back(ToolCallResponse("transfer_to_null", "c1", "{}"));
    // Turn 2: continue with a text response (runner should not crash).
    mock->responses.push_back(TextResponse("Recovered"));

    auto runner = std::make_shared<Runner>(mock, "gpt-4o");
    auto input = std::make_shared<InputString>("hi");

    RunResultOrError result = runner->Run(agent, input);
    ASSERT_FALSE(result.HasError());
    EXPECT_EQ(result.result->final_output, json("Recovered"));
}

// ==========================================================================
// 10. Runner::Run — Options pass-through
// ==========================================================================

TEST(RunnerTest, OptionsPassThrough) {
    auto mock = std::make_shared<foresthub::tests::MockLLMClient>();
    mock->responses.push_back(TextResponse("Done"));

    auto runner = std::make_shared<Runner>(mock, "gpt-4o");
    auto agent = std::make_shared<Agent>("a");
    agent->WithOptions(Options().WithTemperature(0.7f).WithMaxTokens(512));
    auto input = std::make_shared<InputString>("hi");

    RunResultOrError result = runner->Run(agent, input);
    ASSERT_FALSE(result.HasError());

    ASSERT_EQ(mock->captured_requests.size(), 1u);
    const Options& opts = mock->captured_requests[0].options;
    EXPECT_TRUE(opts.temperature.HasValue());
    EXPECT_FLOAT_EQ(*opts.temperature, 0.7f);
    EXPECT_TRUE(opts.max_tokens.HasValue());
    EXPECT_EQ(*opts.max_tokens, 512);
}

TEST(RunnerTest, DefaultOptionsPassThrough) {
    auto mock = std::make_shared<foresthub::tests::MockLLMClient>();
    mock->responses.push_back(TextResponse("Done"));

    auto runner = std::make_shared<Runner>(mock, "gpt-4o");
    auto agent = std::make_shared<Agent>("a");
    // No WithOptions — defaults should be empty.
    auto input = std::make_shared<InputString>("hi");

    RunResultOrError result = runner->Run(agent, input);
    ASSERT_FALSE(result.HasError());

    ASSERT_EQ(mock->captured_requests.size(), 1u);
    const Options& opts = mock->captured_requests[0].options;
    EXPECT_FALSE(opts.temperature.HasValue());
    EXPECT_FALSE(opts.max_tokens.HasValue());
}

TEST(RunnerTest, HandoffOptionsSwitch) {
    auto mock = std::make_shared<foresthub::tests::MockLLMClient>();

    auto agent_b = std::make_shared<Agent>("agent-b");
    agent_b->WithInstructions("I am B").WithOptions(Options().WithTemperature(0.9f));

    auto agent_a = std::make_shared<Agent>("agent-a");
    agent_a->WithOptions(Options().WithTemperature(0.5f));
    auto handoff_tool = NewHandoff("transfer_to_b", "Transfer to B", agent_b);
    agent_a->AddTool(handoff_tool);

    // Turn 1: agent-a invokes handoff.
    mock->responses.push_back(ToolCallResponse("transfer_to_b", "c1", "{}"));
    // Turn 2: agent-b returns text.
    mock->responses.push_back(TextResponse("Hello from B"));

    auto runner = std::make_shared<Runner>(mock, "gpt-4o");
    auto input = std::make_shared<InputString>("hi");

    RunResultOrError result = runner->Run(agent_a, input);
    ASSERT_FALSE(result.HasError());

    ASSERT_EQ(mock->captured_requests.size(), 2u);
    // Agent A's turn: temperature 0.5.
    EXPECT_TRUE(mock->captured_requests[0].options.temperature.HasValue());
    EXPECT_FLOAT_EQ(*mock->captured_requests[0].options.temperature, 0.5f);
    // Agent B's turn: temperature 0.9.
    EXPECT_TRUE(mock->captured_requests[1].options.temperature.HasValue());
    EXPECT_FLOAT_EQ(*mock->captured_requests[1].options.temperature, 0.9f);
}

// ==========================================================================
// 11. Runner::Run — Unsupported tool execution type
// ==========================================================================

TEST(RunnerTest, UnsupportedToolExecutionType) {
    auto mock = std::make_shared<foresthub::tests::MockLLMClient>();

    auto agent = std::make_shared<Agent>("a");
    agent->AddTool(std::make_shared<UnsupportedTypeTool>());

    // LLM requests the unsupported tool.
    mock->responses.push_back(ToolCallResponse("unsupported_tool", "c1", "{}"));

    auto runner = std::make_shared<Runner>(mock, "gpt-4o");
    auto input = std::make_shared<InputString>("hi");

    RunResultOrError result = runner->Run(agent, input);
    EXPECT_TRUE(result.HasError());
    EXPECT_NE(result.error.find("Unsupported tool execution type"), std::string::npos);
    EXPECT_NE(result.error.find("unsupported_tool"), std::string::npos);
}
