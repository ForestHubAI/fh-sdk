#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "foresthub/agent/agent.hpp"
#include "foresthub/agent/handoff.hpp"
#include "foresthub/agent/runner.hpp"
#include "foresthub/client.hpp"
#include "foresthub/core/input.hpp"
#include "foresthub/core/tools.hpp"
#include "foresthub/core/types.hpp"
#include "foresthub/util/json.hpp"
#include "mocks/mock_provider.hpp"

namespace foresthub {
namespace {

using json = nlohmann::json;

// Argument struct for the echo tool (at namespace scope for ADL with NewFunctionTool).
struct EchoArgs {
    std::string msg;
};

// ADL hook required by nlohmann::json — must be snake_case.
void from_json(const json& j, EchoArgs& args) {
    args.msg = j.value("msg", "");
}

// Helper: build a text-only ChatResponse.
std::shared_ptr<core::ChatResponse> MakeTextResponse(const std::string& text) {
    auto resp = std::make_shared<core::ChatResponse>();
    resp->text = text;
    resp->response_id = "resp-id";
    resp->tokens_used = 10;
    return resp;
}

// Helper: build a ChatResponse with tool call requests.
std::shared_ptr<core::ChatResponse> MakeToolCallResponse(const std::string& call_id, const std::string& tool_name,
                                                         const std::string& args_json) {
    auto resp = std::make_shared<core::ChatResponse>();
    core::ToolCallRequest tcr;
    tcr.call_id = call_id;
    tcr.name = tool_name;
    tcr.arguments = args_json;
    resp->tool_call_requests.push_back(tcr);
    return resp;
}

// --- Runner → Client → MockProvider chain ---

TEST(RunnerProviderIntegrationTest, SingleTurnThroughClient) {
    // Set up mock provider with one text response.
    auto mock_provider = std::make_shared<tests::MockProvider>();
    mock_provider->models = {"test-model"};
    mock_provider->responses.push_back(MakeTextResponse("Hello from provider"));

    // Create client and register mock provider.
    auto client = std::make_unique<Client>();
    client->RegisterProvider(mock_provider);

    // Create runner and agent.
    auto runner = std::make_shared<agent::Runner>(std::move(client), "test-model");
    runner->WithMaxTurns(5);

    auto test_agent = std::make_shared<agent::Agent>("test-agent");
    test_agent->WithInstructions("You are a test agent.");

    auto input = std::make_shared<core::InputString>("Hello");
    agent::RunResultOrError result = runner->Run(test_agent, input);

    ASSERT_FALSE(result.HasError()) << result.error;
    ASSERT_TRUE(result.result.HasValue());
    EXPECT_EQ(result.result->final_output, "Hello from provider");
    EXPECT_EQ(result.result->turns, 1);
}

TEST(RunnerProviderIntegrationTest, MultiTurnToolCallingThroughClient) {
    // Turn 1: provider returns a tool call. Turn 2: provider returns text.
    auto mock_provider = std::make_shared<tests::MockProvider>();
    mock_provider->models = {"test-model"};
    mock_provider->responses.push_back(MakeToolCallResponse("call-1", "echo_tool", R"({"msg":"hi"})"));
    mock_provider->responses.push_back(MakeTextResponse("Done"));

    auto client = std::make_unique<Client>();
    client->RegisterProvider(mock_provider);

    auto runner = std::make_shared<agent::Runner>(std::move(client), "test-model");
    runner->WithMaxTurns(5);

    // Create a simple echo tool.
    auto echo_tool = core::NewFunctionTool<EchoArgs, json>(
        "echo_tool", "Echoes back the message",
        json{{"type", "object"}, {"properties", {{"msg", {{"type", "string"}}}}}},
        std::function<json(EchoArgs)>([](const EchoArgs& args) -> json { return json{{"echoed", args.msg}}; }));

    auto test_agent = std::make_shared<agent::Agent>("test-agent");
    test_agent->WithInstructions("You are a test agent.").AddTool(echo_tool);

    auto input = std::make_shared<core::InputString>("Call echo");
    agent::RunResultOrError result = runner->Run(test_agent, input);

    ASSERT_FALSE(result.HasError()) << result.error;
    EXPECT_EQ(result.result->final_output, "Done");
    EXPECT_EQ(result.result->turns, 2);
    EXPECT_EQ(mock_provider->call_count, 2);
}

TEST(RunnerProviderIntegrationTest, HandoffThroughClient) {
    // Agent A gets a handoff tool call → switches to Agent B → B returns text.
    auto mock_provider = std::make_shared<tests::MockProvider>();
    mock_provider->models = {"test-model"};
    // Turn 1 (Agent A): tool call to handoff.
    mock_provider->responses.push_back(MakeToolCallResponse("call-1", "transfer_to_b", "{}"));
    // Turn 2 (Agent B): final text.
    mock_provider->responses.push_back(MakeTextResponse("Hello from B"));

    auto client = std::make_unique<Client>();
    client->RegisterProvider(mock_provider);

    auto runner = std::make_shared<agent::Runner>(std::move(client), "test-model");
    runner->WithMaxTurns(5);

    auto agent_b = std::make_shared<agent::Agent>("agent-b");
    agent_b->WithInstructions("You are agent B.");

    // Use NewHandoff for a real handoff (not AsTool which creates a FunctionTool).
    auto handoff_tool = agent::NewHandoff("transfer_to_b", "Transfer to agent B", agent_b);

    auto agent_a = std::make_shared<agent::Agent>("agent-a");
    agent_a->WithInstructions("You are agent A.");
    agent_a->AddTool(handoff_tool);

    auto input = std::make_shared<core::InputString>("Start");
    agent::RunResultOrError result = runner->Run(agent_a, input);

    ASSERT_FALSE(result.HasError()) << result.error;
    EXPECT_EQ(result.result->final_output, "Hello from B");
    EXPECT_EQ(result.result->last_agent->name(), "agent-b");
}

TEST(RunnerProviderIntegrationTest, ProviderReturnsNull) {
    // Provider Chat() returns nullptr → error.
    auto mock_provider = std::make_shared<tests::MockProvider>();
    mock_provider->models = {"test-model"};
    // No responses queued — will return nullptr.

    auto client = std::make_unique<Client>();
    client->RegisterProvider(mock_provider);

    auto runner = std::make_shared<agent::Runner>(std::move(client), "test-model");
    runner->WithMaxTurns(5);

    auto test_agent = std::make_shared<agent::Agent>("test-agent");
    test_agent->WithInstructions("You are a test agent.");

    auto input = std::make_shared<core::InputString>("Hello");
    agent::RunResultOrError result = runner->Run(test_agent, input);

    EXPECT_TRUE(result.HasError());
}

}  // namespace
}  // namespace foresthub
