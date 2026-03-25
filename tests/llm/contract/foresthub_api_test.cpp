// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "foresthub/llm/input.hpp"
#include "foresthub/llm/serialization.hpp"
#include "foresthub/llm/tools.hpp"
#include "foresthub/llm/types.hpp"
#include "foresthub/util/json.hpp"

using namespace foresthub::llm;
using json = nlohmann::json;

// ===========================================================================
// 1. Request Serialization Contracts
// ===========================================================================

TEST(ForestHubApiContractTest, SimpleTextRequest) {
    ChatRequest req("gpt-4o", std::make_shared<InputString>("Hello"));

    json j = req;

    EXPECT_EQ(j["model"], "gpt-4o");
    ASSERT_TRUE(j["input"].is_object());
    EXPECT_EQ(j["input"]["value"], "Hello");
}

TEST(ForestHubApiContractTest, RequestWithAllOptionalFields) {
    ChatRequest req("gpt-4o", std::make_shared<InputString>("Hi"));
    req.WithSystemPrompt("You are helpful")
        .WithPreviousResponseId("resp_abc")
        .WithResponseFormat(ResponseFormat{"fmt", {{"type", "object"}}, "desc"})
        .AddTool(std::make_shared<WebSearch>())
        .AddFileId("file_1")
        .AddImageId("img_1")
        .AddImageUrl("https://example.com/img.png")
        .WithTemperature(0.7f);

    json j = req;

    EXPECT_TRUE(j.contains("systemPrompt"));
    EXPECT_TRUE(j.contains("previousResponseID"));
    EXPECT_TRUE(j.contains("responseFormat"));
    EXPECT_TRUE(j.contains("tools"));
    EXPECT_TRUE(j.contains("fileIDs"));
    EXPECT_TRUE(j.contains("imageIDs"));
    EXPECT_TRUE(j.contains("imageURLs"));
    EXPECT_TRUE(j.contains("options"));
    EXPECT_EQ(j["systemPrompt"], "You are helpful");
    EXPECT_EQ(j["previousResponseID"], "resp_abc");
}

TEST(ForestHubApiContractTest, RequestWithConversationHistory) {
    auto items = std::make_shared<InputItems>();
    items->PushBack(std::make_shared<InputString>("User message"));

    auto tool_call = std::make_shared<ToolCallRequest>();
    tool_call->call_id = "call_1";
    tool_call->name = "get_weather";
    tool_call->arguments = R"({"city":"Berlin"})";
    items->PushBack(tool_call);

    auto tool_result = std::make_shared<ToolResult>();
    tool_result->call_id = "call_1";
    tool_result->name = "get_weather";
    tool_result->output = json("Sunny, 22C");
    items->PushBack(tool_result);

    ChatRequest req("gpt-4o", items);
    json j = req;

    ASSERT_TRUE(j["input"].is_array());
    ASSERT_EQ(j["input"].size(), 3);

    // UserMessage (InputString)
    EXPECT_EQ(j["input"][0]["value"], "User message");

    // ToolCallRequest
    EXPECT_TRUE(j["input"][1].contains("callId"));
    EXPECT_TRUE(j["input"][1].contains("arguments"));

    // ToolResult
    EXPECT_TRUE(j["input"][2].contains("callId"));
    EXPECT_TRUE(j["input"][2].contains("output"));
}

TEST(ForestHubApiContractTest, RequestToolsSerialization) {
    auto ft = std::make_shared<FunctionTool>();
    ft->name = "calculator";
    ft->description = "Does math";
    ft->parameters = {{"type", "object"}, {"properties", {{"expr", {{"type", "string"}}}}}};

    ChatRequest req("gpt-4o", std::make_shared<InputString>("Hi"));
    req.AddTool(ft).AddTool(std::make_shared<WebSearch>());

    json j = req;

    ASSERT_EQ(j["tools"].size(), 2);

    // FunctionTool
    EXPECT_EQ(j["tools"][0]["name"], "calculator");
    EXPECT_EQ(j["tools"][0]["description"], "Does math");
    EXPECT_TRUE(j["tools"][0].contains("parameters"));
    EXPECT_TRUE(j["tools"][0].contains("type"));

    // WebSearch
    EXPECT_EQ(j["tools"][1]["type"], "web_search");
}

TEST(ForestHubApiContractTest, RequestOptionsOnlySetFields) {
    ChatRequest req("gpt-4o", std::make_shared<InputString>("Hi"));
    req.WithTemperature(0.5f);

    json j = req;

    ASSERT_TRUE(j.contains("options"));
    EXPECT_TRUE(j["options"].contains("temperature"));
    EXPECT_FALSE(j["options"].contains("maxTokens"));
    EXPECT_FALSE(j["options"].contains("topK"));
    EXPECT_FALSE(j["options"].contains("topP"));
    EXPECT_FALSE(j["options"].contains("frequencyPenalty"));
    EXPECT_FALSE(j["options"].contains("presencePenalty"));
    EXPECT_FALSE(j["options"].contains("seed"));
}

// ===========================================================================
// 2. Response Deserialization Contracts
// ===========================================================================

TEST(ForestHubApiContractTest, SimpleTextResponse) {
    json j = {{"text", "Hello from the API"}, {"responseID", "resp_42"}, {"tokensUsed", 15}};

    ChatResponse resp = j.get<ChatResponse>();

    EXPECT_EQ(resp.text, "Hello from the API");
    EXPECT_EQ(resp.response_id, "resp_42");
    EXPECT_EQ(resp.tokens_used, 15);
    EXPECT_TRUE(resp.tool_call_requests.empty());
}

TEST(ForestHubApiContractTest, ToolCallResponse) {
    json j = {{"text", ""},
              {"responseID", "resp_43"},
              {"tokensUsed", 30},
              {"toolCallRequests",
               {{{"callId", "call_abc"}, {"name", "get_weather"}, {"arguments", R"({"city":"Munich"})"}}}}};

    ChatResponse resp = j.get<ChatResponse>();

    ASSERT_EQ(resp.tool_call_requests.size(), 1);
    EXPECT_EQ(resp.tool_call_requests[0].call_id, "call_abc");
    EXPECT_EQ(resp.tool_call_requests[0].name, "get_weather");
    EXPECT_EQ(resp.tool_call_requests[0].arguments, R"({"city":"Munich"})");
}

TEST(ForestHubApiContractTest, ToolCallResponseWithObjectArguments) {
    // When arguments is a JSON object (not a string), it should be dumped to string
    json j = {{"text", ""},
              {"toolCallRequests", {{{"callId", "call_1"}, {"name", "func"}, {"arguments", {{"a", 1}}}}}}};

    ChatResponse resp = j.get<ChatResponse>();

    ASSERT_EQ(resp.tool_call_requests.size(), 1);
    // Object arguments get dumped to string via args.dump()
    EXPECT_EQ(resp.tool_call_requests[0].arguments, R"({"a":1})");
}

TEST(ForestHubApiContractTest, EmptyResponse) {
    json j = json::object();

    ChatResponse resp = j.get<ChatResponse>();

    EXPECT_TRUE(resp.text.empty());
    EXPECT_TRUE(resp.tool_call_requests.empty());
    EXPECT_EQ(resp.tokens_used, 0);
}

TEST(ForestHubApiContractTest, ResponseMissingFields) {
    json j = {{"text", "Only text"}};

    ChatResponse resp = j.get<ChatResponse>();

    EXPECT_EQ(resp.text, "Only text");
    EXPECT_TRUE(resp.response_id.empty());
    EXPECT_EQ(resp.tokens_used, 0);
    EXPECT_TRUE(resp.tool_call_requests.empty());
}

// ===========================================================================
// 3. Roundtrip Contracts
// ===========================================================================

TEST(ForestHubApiContractTest, ChatRequestRoundtrip) {
    ChatRequest original("gpt-4o", std::make_shared<InputString>("Roundtrip test"));
    original.WithSystemPrompt("Be concise").WithTemperature(0.8f).WithMaxTokens(100);

    // Serialize
    json j = original;

    // Deserialize
    ChatRequest restored = j.get<ChatRequest>();

    EXPECT_EQ(restored.model, "gpt-4o");
    EXPECT_EQ(restored.system_prompt, "Be concise");
    ASSERT_NE(restored.input, nullptr);
    ASSERT_EQ(restored.input->GetInputType(), InputType::kString);
    auto input_str = std::static_pointer_cast<InputString>(restored.input);
    EXPECT_EQ(input_str->text, "Roundtrip test");
    ASSERT_TRUE(restored.options.temperature.HasValue());
    EXPECT_FLOAT_EQ(*restored.options.temperature, 0.8f);
    ASSERT_TRUE(restored.options.max_tokens.HasValue());
    EXPECT_EQ(*restored.options.max_tokens, 100);
}

// ===========================================================================
// 3. Wire Format Key Names
// ===========================================================================

TEST(ForestHubApiContractTest, WireFormatKeyNames) {
    // Verify exact camelCase JSON key names match server expectations
    ChatRequest req("gpt-4o", std::make_shared<InputString>("Hi"));
    req.WithSystemPrompt("sys").WithPreviousResponseId("prev_id");

    json j = req;

    // Request keys must be camelCase
    EXPECT_TRUE(j.contains("systemPrompt"));
    EXPECT_FALSE(j.contains("system_prompt"));
    EXPECT_TRUE(j.contains("previousResponseID"));
    EXPECT_FALSE(j.contains("previous_response_id"));

    // Response key names
    json resp_j = {{"text", "ok"},
                   {"responseID", "id_1"},
                   {"tokensUsed", 5},
                   {"toolCallRequests", {{{"callId", "c1"}, {"name", "fn"}, {"arguments", "{}"}}}}};

    ChatResponse resp = resp_j.get<ChatResponse>();

    EXPECT_EQ(resp.response_id, "id_1");
    EXPECT_EQ(resp.tokens_used, 5);
    EXPECT_EQ(resp.tool_call_requests[0].call_id, "c1");
}
