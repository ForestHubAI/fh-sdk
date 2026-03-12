// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "foresthub/core/tools.hpp"

#include <gtest/gtest.h>

#include <functional>
#include <memory>
#include <string>

#include "foresthub/util/json.hpp"

using namespace foresthub::core;
using json = nlohmann::json;

// ==========================================
// 0. Helper Structs for Factory Tests
// ==========================================

// A dummy struct to test argument parsing in NewFunctionTool
struct WeatherArgs {
    std::string city;
};

// ADL hook required by nlohmann::json — must be snake_case.
void from_json(const json& j, WeatherArgs& args) {
    args.city = j.value("city", "");
}

// ==========================================
// 1. Tests for Data Structures (Requests)
// ==========================================

TEST(ToolCallRequestTest, StructureAndToString) {
    ToolCallRequest req;
    req.call_id = "call_123";
    req.name = "get_weather";
    req.arguments = "{\"city\": \"Berlin\"}";

    EXPECT_EQ(req.call_id, "call_123");
    EXPECT_EQ(req.name, "get_weather");
    EXPECT_EQ(req.arguments, "{\"city\": \"Berlin\"}");

    std::string expected = "Function tool call: get_weather with arguments {\"city\": \"Berlin\"}";
    EXPECT_EQ(req.ToString(), expected);
}

TEST(ToolCallRequestTest, Inheritance) {
    auto req = std::make_shared<ToolCallRequest>();
    req->call_id = "call_123";
    req->name = "test_func_call";

    std::shared_ptr<InputItem> item = req;

    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->ToString().find("test_func_call") != std::string::npos);
}

// ==========================================
// 2. Tests for ToolResult (Polymorphism)
// ==========================================

TEST(ToolResultTest, StringOutput) {
    ToolResult res;
    res.call_id = "call_123";
    res.name = "echo";
    res.output = json("Hello Output");

    std::string str = res.ToString();
    EXPECT_EQ(str, "Function echo returned: Hello Output");
}

TEST(ToolResultTest, IntOutput) {
    ToolResult res;
    res.call_id = "call_123";
    res.name = "add";
    res.output = json(42);

    std::string str = res.ToString();
    EXPECT_EQ(str, "Function add returned: 42");
}

TEST(ToolResultTest, BoolOutput) {
    ToolResult res;
    res.call_id = "call_123";
    res.name = "is_valid";

    res.output = json(true);
    EXPECT_EQ(res.ToString(), "Function is_valid returned: true");

    res.output = json(false);
    EXPECT_EQ(res.ToString(), "Function is_valid returned: false");
}

TEST(ToolResultTest, JsonOutput) {
    ToolResult res;
    res.call_id = "call_123";
    res.name = "get_json";
    res.output = json{{"key", "value"}};

    std::string str = res.ToString();
    EXPECT_EQ(str, "Function get_json returned: {\"key\":\"value\"}");
}

TEST(ToolResultTest, ArrayOutput) {
    ToolResult res;
    res.call_id = "call_123";
    res.name = "get_list";
    res.output = json::array({1, 2, 3});

    std::string str = res.ToString();
    EXPECT_EQ(str, "Function get_list returned: [1,2,3]");
}

// ==========================================
// 3. Tests for ExternalToolBase
// ==========================================

TEST(ExternalToolBaseTest, BasicProperties) {
    FunctionTool tool;
    tool.name = "calculator";
    tool.description = "calc things";
    tool.parameters = {{"type", "object"}};

    EXPECT_EQ(tool.ToolName(), "calculator");
    EXPECT_EQ(tool.ToolDescription(), "calc things");
    EXPECT_FALSE(tool.ToolParameters().empty());
    EXPECT_TRUE(tool.IsExternal());
    EXPECT_EQ(tool.GetToolType(), ToolType::kFunction);
}

// ==========================================
// 4. Tests for FunctionTool Factory
// ==========================================

TEST(FunctionToolFactoryTest, CreationAndExecution_HappyPath) {
    std::function<std::string(WeatherArgs)> handler = [](const WeatherArgs& args) -> std::string {
        return "Weather in " + args.city + " is sunny";
    };

    json schema = {{"type", "object"}, {"properties", {{"city", {{"type", "string"}}}}}};

    auto tool = NewFunctionTool<WeatherArgs, std::string>("get_weather", "Gets weather for city", schema, handler);

    ASSERT_NE(tool, nullptr);
    EXPECT_EQ(tool->ToolName(), "get_weather");
    EXPECT_EQ(tool->ToolDescription(), "Gets weather for city");
    EXPECT_FALSE(tool->ToolParameters().empty());

    std::string call_args = R"({"city": "Munich"})";
    json result = tool->tool_call(call_args);

    // json stores the return value directly
    ASSERT_TRUE(result.is_string());
    EXPECT_EQ(result.get<std::string>(), "Weather in Munich is sunny");
}

TEST(FunctionToolFactoryTest, NullJsonSchema) {
    std::function<int(WeatherArgs)> handler = [](const WeatherArgs& /*args*/) -> int { return 0; };

    auto tool = NewFunctionTool<WeatherArgs, int>("broken_tool", "desc", json(), handler);

    // The factory falls back to an empty object for null schema
    EXPECT_TRUE(tool->ToolParameters().is_object());
    EXPECT_TRUE(tool->ToolParameters().empty());
}

TEST(FunctionToolFactoryTest, ExecutionWithInvalidArguments) {
    std::function<std::string(WeatherArgs)> handler = [](const WeatherArgs& /*args*/) -> std::string { return "ok"; };

    auto tool = NewFunctionTool<WeatherArgs, std::string>("test_tool", "desc", json::object(), handler);

    std::string malformed_args = "{ city: 'Berlin' ";  // Invalid JSON syntax
    json result = tool->tool_call(malformed_args);

    ASSERT_TRUE(result.is_string());
    std::string error_msg = result.get<std::string>();
    EXPECT_TRUE(error_msg.find("Error: Tool arguments for test_tool were not valid JSON") != std::string::npos);
}

// ==========================================
// 5. Tests for Internal Tools (WebSearch)
// ==========================================

TEST(InternalToolsTest, WebSearch) {
    WebSearch ws;
    Tool& tool_ref = ws;
    EXPECT_EQ(tool_ref.ToolName(), "web_search");
}

TEST(InternalToolsTest, WebSearchToolCall) {
    WebSearchToolCall call;
    call.query = "golang tutorials";

    InternalToolCall& internal_call = call;

    EXPECT_EQ(internal_call.ToolName(), "web_search");
    EXPECT_EQ(call.query, "golang tutorials");
}

TEST(InternalToolsTest, WebSearchIsNotExternal) {
    WebSearch ws;
    EXPECT_FALSE(ws.IsExternal());
}

// ==========================================
// 6. Tests for ToJson on Data Structures
// ==========================================

TEST(ToolCallRequestTest, ToJsonValidArguments) {
    ToolCallRequest req;
    req.call_id = "call_1";
    req.name = "func";
    req.arguments = R"({"key":"value"})";

    json j;
    req.ToJson(j);

    EXPECT_EQ(j["callId"], "call_1");
    EXPECT_EQ(j["name"], "func");
    // Valid JSON arguments are parsed as JSON object
    EXPECT_EQ(j["arguments"]["key"], "value");
}

TEST(ToolCallRequestTest, ToJsonInvalidArguments) {
    ToolCallRequest req;
    req.call_id = "call_2";
    req.name = "broken";
    req.arguments = "not valid json {";

    json j;
    req.ToJson(j);

    EXPECT_EQ(j["callId"], "call_2");
    // Invalid JSON falls back to raw string
    EXPECT_EQ(j["arguments"], "not valid json {");
}

TEST(ToolResultTest, ToJson) {
    ToolResult res;
    res.call_id = "call_3";
    res.name = "echo";
    res.output = json("hello");

    json j;
    res.ToJson(j);

    EXPECT_EQ(j["callId"], "call_3");
    EXPECT_EQ(j["name"], "echo");
    EXPECT_EQ(j["output"], "hello");
}

TEST(ExternalToolBaseTest, ToJson) {
    FunctionTool ft;
    ft.name = "calc";
    ft.description = "Does math";
    ft.parameters = {{"type", "object"}};

    json j;
    ft.ToJson(j);

    EXPECT_EQ(j["type"], "external");
    EXPECT_EQ(j["name"], "calc");
    EXPECT_EQ(j["description"], "Does math");
    EXPECT_EQ(j["parameters"]["type"], "object");
}
