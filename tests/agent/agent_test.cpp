#include "foresthub/agent/agent.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "foresthub/agent/runner.hpp"
#include "foresthub/core/options.hpp"
#include "foresthub/core/tools.hpp"
#include "foresthub/util/json.hpp"
#include "mocks/mock_llm_client.hpp"

using namespace foresthub::agent;
using namespace foresthub::core;
using json = nlohmann::json;

// Helper struct for tool creation (same pattern as tools_test.cpp).
struct WeatherArgs {
    std::string city;
};

// ADL hook required by nlohmann::json — must be snake_case.
void from_json(const json& j, WeatherArgs& args) {
    args.city = j.value("city", "");
}

static std::shared_ptr<FunctionTool> MakeWeatherTool() {
    json schema = {{"type", "object"}, {"properties", {{"city", {{"type", "string"}}}}}};
    std::function<std::string(WeatherArgs)> handler = [](const WeatherArgs& args) -> std::string {
        return "Sunny in " + args.city;
    };
    return NewFunctionTool<WeatherArgs, std::string>("get_weather", "Gets weather", schema, handler);
}

// ==========================================================================
// 1. Construction and Fluent API
// ==========================================================================

TEST(AgentTest, Construction) {
    Agent agent("test-agent");
    EXPECT_EQ(agent.name(), "test-agent");
    EXPECT_TRUE(agent.instructions().empty());
    EXPECT_TRUE(agent.tools().empty());
    EXPECT_FALSE(agent.response_format().HasValue());
}

TEST(AgentTest, WithInstructions) {
    Agent agent("a");
    Agent& ref = agent.WithInstructions("Be helpful");
    EXPECT_EQ(agent.instructions(), "Be helpful");
    EXPECT_EQ(&ref, &agent);
}

TEST(AgentTest, WithResponseFormat) {
    Agent agent("a");
    ResponseFormat fmt;
    fmt.name = "json_output";
    fmt.schema = json{{"type", "object"}};
    agent.WithResponseFormat(fmt);
    EXPECT_TRUE(agent.response_format().HasValue());
    EXPECT_EQ(agent.response_format()->name, "json_output");
}

TEST(AgentTest, DefaultOptions) {
    Agent agent("a");
    const Options& opts = agent.options();
    EXPECT_FALSE(opts.temperature.HasValue());
    EXPECT_FALSE(opts.max_tokens.HasValue());
    EXPECT_FALSE(opts.top_k.HasValue());
    EXPECT_FALSE(opts.top_p.HasValue());
    EXPECT_FALSE(opts.frequency_penalty.HasValue());
    EXPECT_FALSE(opts.presence_penalty.HasValue());
    EXPECT_FALSE(opts.seed.HasValue());
}

TEST(AgentTest, WithOptions) {
    Agent agent("a");
    Agent& ref = agent.WithOptions(Options().WithTemperature(0.7f).WithMaxTokens(512));
    EXPECT_EQ(&ref, &agent);
    EXPECT_TRUE(agent.options().temperature.HasValue());
    EXPECT_FLOAT_EQ(*agent.options().temperature, 0.7f);
    EXPECT_TRUE(agent.options().max_tokens.HasValue());
    EXPECT_EQ(*agent.options().max_tokens, 512);
}

TEST(AgentTest, WithOptionsFluentChain) {
    Agent agent("a");
    agent.WithInstructions("Be helpful").WithOptions(Options().WithTemperature(0.5f)).AddTool(MakeWeatherTool());
    EXPECT_EQ(agent.instructions(), "Be helpful");
    EXPECT_FLOAT_EQ(*agent.options().temperature, 0.5f);
    EXPECT_EQ(agent.tools().size(), 1u);
}

TEST(AgentTest, WithTools) {
    Agent agent("a");
    std::vector<std::shared_ptr<Tool>> tools;
    tools.push_back(MakeWeatherTool());
    tools.push_back(std::make_shared<WebSearch>());
    agent.WithTools(tools);
    EXPECT_EQ(agent.tools().size(), 2u);
}

TEST(AgentTest, AddTool) {
    Agent agent("a");
    Agent& ref = agent.AddTool(MakeWeatherTool());
    EXPECT_EQ(agent.tools().size(), 1u);
    EXPECT_EQ(&ref, &agent);
    agent.AddTool(std::make_shared<WebSearch>());
    EXPECT_EQ(agent.tools().size(), 2u);
}

TEST(AgentTest, FluentChaining) {
    Agent agent("a");
    agent.WithInstructions("instructions").AddTool(MakeWeatherTool()).AddTool(std::make_shared<WebSearch>());
    EXPECT_EQ(agent.instructions(), "instructions");
    EXPECT_EQ(agent.tools().size(), 2u);
}

// ==========================================================================
// 2. FindExternalTool
// ==========================================================================

TEST(AgentTest, FindExternalTool_Found) {
    Agent agent("a");
    agent.AddTool(MakeWeatherTool());
    std::shared_ptr<ExternalTool> found = agent.FindExternalTool("get_weather");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->ToolName(), "get_weather");
}

TEST(AgentTest, FindExternalTool_NotFound) {
    Agent agent("a");
    agent.AddTool(MakeWeatherTool());
    EXPECT_EQ(agent.FindExternalTool("nonexistent"), nullptr);
}

TEST(AgentTest, FindExternalTool_SkipsNonExternal) {
    Agent agent("a");
    agent.AddTool(std::make_shared<WebSearch>());
    // WebSearch has ToolName()="web_search" but IsExternal()=false.
    EXPECT_EQ(agent.FindExternalTool("web_search"), nullptr);
}

TEST(AgentTest, FindExternalTool_EmptyTools) {
    Agent agent("a");
    EXPECT_EQ(agent.FindExternalTool("anything"), nullptr);
}

// ==========================================================================
// 3. AsTool
// ==========================================================================

TEST(AgentTest, AsTool_Creation) {
    auto mock_client = std::make_shared<foresthub::tests::MockLLMClient>();
    auto runner = std::make_shared<Runner>(mock_client, "test-model");

    Agent agent("helper");
    agent.WithInstructions("You help");

    std::shared_ptr<Tool> tool = agent.AsTool("ask_helper", "Asks the helper agent", runner);
    ASSERT_NE(tool, nullptr);
    EXPECT_EQ(tool->ToolName(), "ask_helper");
    EXPECT_TRUE(tool->IsExternal());
    EXPECT_EQ(tool->GetToolType(), ToolType::kFunction);

    auto ft = std::static_pointer_cast<FunctionTool>(tool);
    EXPECT_EQ(ft->ToolDescription(), "Asks the helper agent");
    EXPECT_FALSE(ft->ToolParameters().empty());
}

TEST(AgentTest, AsTool_Execution) {
    auto mock_client = std::make_shared<foresthub::tests::MockLLMClient>();

    // Mock returns a text response.
    auto response = std::make_shared<ChatResponse>();
    response->text = "Agent result";
    mock_client->responses.push_back(response);

    auto runner = std::make_shared<Runner>(mock_client, "test-model");

    Agent agent("helper");
    agent.WithInstructions("You help");

    std::shared_ptr<Tool> tool = agent.AsTool("ask_helper", "Asks the helper agent", runner);
    auto ft = std::static_pointer_cast<FunctionTool>(tool);

    json result = ft->tool_call(R"({"prompt": "Hello"})");
    EXPECT_EQ(result, json("Agent result"));
}

TEST(AgentTest, AsTool_Error) {
    auto mock_client = std::make_shared<foresthub::tests::MockLLMClient>();
    // No responses queued — Chat() returns nullptr → network error.

    auto runner = std::make_shared<Runner>(mock_client, "test-model");

    Agent agent("helper");

    std::shared_ptr<Tool> tool = agent.AsTool("ask_helper", "Asks the helper agent", runner);
    auto ft = std::static_pointer_cast<FunctionTool>(tool);

    json result = ft->tool_call(R"({"prompt": "Hello"})");
    ASSERT_TRUE(result.is_string());
    std::string msg = result.get<std::string>();
    EXPECT_TRUE(msg.find("Error running agent tool") != std::string::npos);
}
