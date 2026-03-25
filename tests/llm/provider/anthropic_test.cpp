// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "foresthub/llm/config.hpp"
#include "llm/provider/remote/anthropic/provider.hpp"
#include "mocks/mock_http_client.hpp"

namespace foresthub {
namespace llm {
namespace anthropic {
namespace {

using hal::HttpResponse;
using json = nlohmann::json;

// Helper: create an AnthropicProvider with the given MockHttpClient and config.
struct TestFixture {
    std::shared_ptr<tests::MockHttpClient> mock_http = std::make_shared<tests::MockHttpClient>();
    llm::ProviderConfig cfg;

    TestFixture() {
        cfg.api_key = "test-anthropic-key";
        cfg.supported_models = {"claude-sonnet-4-6", "claude-haiku-4-5"};
    }

    AnthropicProvider MakeProvider() { return AnthropicProvider(cfg, mock_http); }

    // A valid Anthropic Messages API response body with text output.
    static std::string TextResponseBody() {
        json j = {{"id", "msg_abc123"},
                  {"type", "message"},
                  {"role", "assistant"},
                  {"stop_reason", "end_turn"},
                  {"content", {{{"type", "text"}, {"text", "Hello world"}}}},
                  {"usage", {{"input_tokens", 10}, {"output_tokens", 32}}}};
        return j.dump();
    }

    // A response body with a tool_use content block.
    static std::string ToolCallResponseBody() {
        json j = {
            {"id", "msg_tool1"},
            {"type", "message"},
            {"role", "assistant"},
            {"stop_reason", "tool_use"},
            {"content",
             {{{"type", "tool_use"}, {"id", "toolu_abc"}, {"name", "get_weather"}, {"input", {{"city", "Berlin"}}}}}},
            {"usage", {{"input_tokens", 15}, {"output_tokens", 20}}}};
        return j.dump();
    }

    // A response body with both text and tool_use content blocks.
    static std::string MixedContentResponseBody() {
        json j = {
            {"id", "msg_mixed1"},
            {"type", "message"},
            {"role", "assistant"},
            {"stop_reason", "tool_use"},
            {"content",
             {{{"type", "text"}, {"text", "Let me check the weather."}},
              {{"type", "tool_use"}, {"id", "toolu_xyz"}, {"name", "get_weather"}, {"input", {{"city", "Paris"}}}}}},
            {"usage", {{"input_tokens", 20}, {"output_tokens", 40}}}};
        return j.dump();
    }

    // A response body with stop_reason "refusal".
    static std::string RefusalResponseBody() {
        json j = {{"id", "msg_ref1"},         {"type", "message"},
                  {"role", "assistant"},      {"stop_reason", "refusal"},
                  {"content", json::array()}, {"usage", {{"input_tokens", 5}, {"output_tokens", 0}}}};
        return j.dump();
    }

    // A response body with stop_reason "max_tokens" and partial text.
    static std::string MaxTokensResponseBody() {
        json j = {{"id", "msg_max1"},
                  {"type", "message"},
                  {"role", "assistant"},
                  {"stop_reason", "max_tokens"},
                  {"content", {{{"type", "text"}, {"text", "This is partial"}}}},
                  {"usage", {{"input_tokens", 10}, {"output_tokens", 100}}}};
        return j.dump();
    }
};

// --- Construction & Identity ---

TEST(AnthropicProvider, Construction) {
    TestFixture f;
    AnthropicProvider provider = f.MakeProvider();

    EXPECT_EQ(provider.ProviderId(), "anthropic");
}

TEST(AnthropicProvider, ConstructionStripsTrailingSlash) {
    TestFixture f;
    f.cfg.base_url = "https://api.anthropic.com/";
    AnthropicProvider provider = f.MakeProvider();

    f.mock_http->get_responses.push_back({200, "{}", {}});
    provider.Health();
    EXPECT_EQ(f.mock_http->last_url, "https://api.anthropic.com/v1/models");
}

TEST(AnthropicProvider, DefaultBaseUrl) {
    llm::ProviderConfig cfg;
    cfg.api_key = "test-key";
    auto mock = std::make_shared<tests::MockHttpClient>();
    mock->get_responses.push_back({200, "{}", {}});
    AnthropicProvider provider(cfg, mock);
    provider.Health();
    EXPECT_EQ(mock->last_url, "https://api.anthropic.com/v1/models");
}

// --- SupportsModel ---

TEST(AnthropicProvider, SupportsModelFound) {
    TestFixture f;
    AnthropicProvider provider = f.MakeProvider();

    EXPECT_TRUE(provider.SupportsModel("claude-sonnet-4-6"));
    EXPECT_TRUE(provider.SupportsModel("claude-haiku-4-5"));
}

TEST(AnthropicProvider, SupportsModelNotFound) {
    TestFixture f;
    AnthropicProvider provider = f.MakeProvider();

    EXPECT_FALSE(provider.SupportsModel("gpt-4o"));
}

TEST(AnthropicProvider, SupportsModelEmptyListAcceptsAll) {
    TestFixture f;
    f.cfg.supported_models.clear();
    AnthropicProvider provider = f.MakeProvider();

    EXPECT_TRUE(provider.SupportsModel("claude-sonnet-4-6"));
    EXPECT_TRUE(provider.SupportsModel("any-model"));
}

// --- Health ---

TEST(AnthropicProvider, HealthSuccess) {
    TestFixture f;
    f.mock_http->get_responses.push_back({200, "{}", {}});
    AnthropicProvider provider = f.MakeProvider();

    std::string err = provider.Health();
    EXPECT_TRUE(err.empty());
}

TEST(AnthropicProvider, HealthFailure) {
    TestFixture f;
    f.mock_http->get_responses.push_back({401, "Unauthorized", {}});
    AnthropicProvider provider = f.MakeProvider();

    std::string err = provider.Health();
    EXPECT_FALSE(err.empty());
    EXPECT_NE(err.find("401"), std::string::npos);
}

TEST(AnthropicProvider, HealthUrlCorrect) {
    TestFixture f;
    f.mock_http->get_responses.push_back({200, "{}", {}});
    AnthropicProvider provider = f.MakeProvider();

    provider.Health();
    EXPECT_EQ(f.mock_http->last_url, "https://api.anthropic.com/v1/models");
}

TEST(AnthropicProvider, HealthHeadersContainApiKey) {
    TestFixture f;
    f.mock_http->get_responses.push_back({200, "{}", {}});
    AnthropicProvider provider = f.MakeProvider();

    provider.Health();
    EXPECT_EQ(f.mock_http->last_headers["x-api-key"], "test-anthropic-key");
    EXPECT_EQ(f.mock_http->last_headers["anthropic-version"], "2023-06-01");
    // Must NOT use Authorization: Bearer
    EXPECT_EQ(f.mock_http->last_headers.count("Authorization"), 0u);
}

// --- Chat: Happy Path ---

TEST(AnthropicProvider, ChatTextResponse) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->text, "Hello world");
    EXPECT_EQ(resp->response_id, "msg_abc123");
    EXPECT_EQ(resp->tokens_used, 42);
}

TEST(AnthropicProvider, ChatUrlCorrect) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    provider.Chat(req);

    EXPECT_EQ(f.mock_http->last_url, "https://api.anthropic.com/v1/messages");
}

TEST(AnthropicProvider, ChatHeadersCorrect) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    provider.Chat(req);

    EXPECT_EQ(f.mock_http->last_headers["Content-Type"], "application/json");
    EXPECT_EQ(f.mock_http->last_headers["x-api-key"], "test-anthropic-key");
    EXPECT_EQ(f.mock_http->last_headers["anthropic-version"], "2023-06-01");
}

// --- Chat: Request JSON ---

TEST(AnthropicProvider, ChatWithSystemPrompt) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    req.WithSystemPrompt("You are helpful");
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    EXPECT_EQ(body.value("system", ""), "You are helpful");
}

TEST(AnthropicProvider, ChatWithOptions) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    req.WithTemperature(0.7f).WithMaxTokens(1000).WithTopP(0.9f).WithTopK(40);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    EXPECT_EQ(body.value("max_tokens", 0), 1000);
    EXPECT_FLOAT_EQ(body.value("temperature", 0.0f), 0.7f);
    EXPECT_FLOAT_EQ(body.value("top_p", 0.0f), 0.9f);
    EXPECT_EQ(body.value("top_k", 0), 40);
}

TEST(AnthropicProvider, ChatWithTools) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Get weather");
    llm::ChatRequest req("claude-sonnet-4-6", input);

    auto fn_tool = std::make_shared<llm::FunctionTool>();
    fn_tool->name = "get_weather";
    fn_tool->description = "Get the weather";
    fn_tool->parameters = json{{"type", "object"}, {"properties", {{"city", {{"type", "string"}}}}}};
    req.AddTool(fn_tool);

    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    ASSERT_TRUE(body.contains("tools"));
    ASSERT_EQ(body["tools"].size(), 1u);

    // Anthropic uses input_schema (not parameters)
    EXPECT_EQ(body["tools"][0].value("name", ""), "get_weather");
    EXPECT_EQ(body["tools"][0].value("description", ""), "Get the weather");
    EXPECT_TRUE(body["tools"][0].contains("input_schema"));
    EXPECT_FALSE(body["tools"][0].contains("parameters"));
}

TEST(AnthropicProvider, ChatWithResponseFormat) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Give me JSON");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    llm::ResponseFormat fmt;
    fmt.name = "my_schema";
    fmt.schema = json{{"type", "object"}, {"properties", {{"answer", {{"type", "string"}}}}}};
    req.WithResponseFormat(fmt);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    ASSERT_TRUE(body.contains("output_config"));
    ASSERT_TRUE(body["output_config"].contains("format"));

    const json& format = body["output_config"]["format"];
    EXPECT_EQ(format.value("type", ""), "json_schema");
    EXPECT_TRUE(format.contains("schema"));
    // Anthropic format must not contain "name" (unlike OpenAI)
    EXPECT_FALSE(format.contains("name"));
    // Deprecated top-level "output_format" must not be emitted
    EXPECT_FALSE(body.contains("output_format"));
}

TEST(AnthropicProvider, ChatToolSchemaStrictified) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Get weather");
    llm::ChatRequest req("claude-sonnet-4-6", input);

    // Minimal schema (no type, no required, no additionalProperties)
    auto fn_tool = std::make_shared<llm::FunctionTool>();
    fn_tool->name = "get_weather";
    fn_tool->description = "Get the weather";
    fn_tool->parameters = json{{"city", {{"type", "string"}}}, {"units", {{"type", "string"}}}};
    req.AddTool(fn_tool);

    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    const json& schema = body["tools"][0]["input_schema"];

    // NormalizeSchema: wrapped into full format
    EXPECT_EQ(schema.value("type", ""), "object");
    EXPECT_TRUE(schema.contains("properties"));
    // EnsureAllRequired: all keys in required
    ASSERT_TRUE(schema.contains("required"));
    EXPECT_EQ(schema["required"].size(), 2u);
    // SetNoAdditionalProperties: additionalProperties:false
    EXPECT_EQ(schema["additionalProperties"], false);
}

TEST(AnthropicProvider, ChatResponseFormatSchemaStrictified) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Give me JSON");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    llm::ResponseFormat fmt;
    fmt.name = "test";
    fmt.schema = json{{"type", "object"}, {"properties", {{"answer", {{"type", "string"}}}}}};
    req.WithResponseFormat(fmt);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    const json& schema = body["output_config"]["format"]["schema"];

    ASSERT_TRUE(schema.contains("required"));
    EXPECT_EQ(schema["required"].size(), 1u);
    EXPECT_EQ(schema["additionalProperties"], false);
}

TEST(AnthropicProvider, ChatToolResultInput) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto items = std::make_shared<llm::InputItems>();

    auto tcr = std::make_shared<llm::ToolCallRequest>();
    tcr->call_id = "toolu_1";
    tcr->name = "get_weather";
    tcr->arguments = "{\"city\":\"Berlin\"}";
    items->PushBack(tcr);

    auto tr = std::make_shared<llm::ToolResult>();
    tr->call_id = "toolu_1";
    tr->name = "get_weather";
    tr->output = json("sunny, 22C");
    items->PushBack(tr);

    llm::ChatRequest req("claude-sonnet-4-6", items);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    ASSERT_TRUE(body["messages"].is_array());
    ASSERT_EQ(body["messages"].size(), 2u);

    // Tool call request → assistant message with tool_use content
    EXPECT_EQ(body["messages"][0].value("role", ""), "assistant");
    ASSERT_TRUE(body["messages"][0]["content"].is_array());
    EXPECT_EQ(body["messages"][0]["content"][0].value("type", ""), "tool_use");
    EXPECT_EQ(body["messages"][0]["content"][0].value("id", ""), "toolu_1");

    // Tool result → user message with tool_result content
    EXPECT_EQ(body["messages"][1].value("role", ""), "user");
    ASSERT_TRUE(body["messages"][1]["content"].is_array());
    EXPECT_EQ(body["messages"][1]["content"][0].value("type", ""), "tool_result");
    EXPECT_EQ(body["messages"][1]["content"][0].value("tool_use_id", ""), "toolu_1");
}

TEST(AnthropicProvider, ChatMaxTokensDefault) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    // No WithMaxTokens — should default to 4096
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    EXPECT_EQ(body.value("max_tokens", 0), 4096);
}

TEST(AnthropicProvider, ChatMaxTokensFromOptions) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    req.WithMaxTokens(2048);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    EXPECT_EQ(body.value("max_tokens", 0), 2048);
}

// --- Chat: Response Parsing ---

TEST(AnthropicProvider, ChatToolCallResponse) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::ToolCallResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("What is the weather?");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    ASSERT_EQ(resp->tool_call_requests.size(), 1u);
    EXPECT_EQ(resp->tool_call_requests[0].call_id, "toolu_abc");
    EXPECT_EQ(resp->tool_call_requests[0].name, "get_weather");
    EXPECT_EQ(resp->tool_call_requests[0].arguments, "{\"city\":\"Berlin\"}");
    EXPECT_EQ(resp->response_id, "msg_tool1");
}

TEST(AnthropicProvider, ChatTokenUsage) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    // 10 input + 32 output = 42
    EXPECT_EQ(resp->tokens_used, 42);
}

TEST(AnthropicProvider, ChatResponseId) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->response_id, "msg_abc123");
}

TEST(AnthropicProvider, ChatMixedContentBlocks) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::MixedContentResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Weather?");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->text, "Let me check the weather.");
    ASSERT_EQ(resp->tool_call_requests.size(), 1u);
    EXPECT_EQ(resp->tool_call_requests[0].name, "get_weather");
}

// --- Chat: stop_reason ---

TEST(AnthropicProvider, ChatRefusalReturnsNullptr) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::RefusalResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Something bad");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    EXPECT_EQ(resp, nullptr);
}

TEST(AnthropicProvider, ChatMaxTokensStillReturnsText) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::MaxTokensResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Tell me a story");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->text, "This is partial");
}

// --- Chat: WebSearch Filtering ---

TEST(AnthropicProvider, ChatWebSearchToolFiltered) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Search the web");
    llm::ChatRequest req("claude-sonnet-4-6", input);

    // Add both a function tool and WebSearch
    auto fn_tool = std::make_shared<llm::FunctionTool>();
    fn_tool->name = "get_weather";
    fn_tool->description = "Get the weather";
    fn_tool->parameters = json{{"type", "object"}, {"properties", {{"city", {{"type", "string"}}}}}};
    req.AddTool(fn_tool);
    req.AddTool(std::make_shared<llm::WebSearch>());

    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    ASSERT_TRUE(body.contains("tools"));
    // Only the function tool should be present — WebSearch is filtered
    ASSERT_EQ(body["tools"].size(), 1u);
    EXPECT_EQ(body["tools"][0].value("name", ""), "get_weather");
}

// --- Chat: Error Paths ---

TEST(AnthropicProvider, ChatHttpError) {
    TestFixture f;
    f.mock_http->post_responses.push_back({400, "bad request", {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    EXPECT_EQ(resp, nullptr);
}

TEST(AnthropicProvider, ChatMalformedJson) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, "not valid json {{{", {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    EXPECT_EQ(resp, nullptr);
}

// --- Retry Logic ---

TEST(AnthropicProvider, ChatRetryOn429) {
    TestFixture f;
    f.mock_http->post_responses.push_back({429, "rate limited", {}});
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->text, "Hello world");
    EXPECT_EQ(f.mock_http->post_call_count, 2);
}

TEST(AnthropicProvider, ChatNoRetryOn4xx) {
    TestFixture f;
    f.mock_http->post_responses.push_back({403, "forbidden", {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    provider.Chat(req);

    EXPECT_EQ(f.mock_http->post_call_count, 1);
}

TEST(AnthropicProvider, ChatRetryOn408) {
    TestFixture f;
    f.mock_http->post_responses.push_back({408, "request timeout", {}});
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->text, "Hello world");
    EXPECT_EQ(f.mock_http->post_call_count, 2);
}

TEST(AnthropicProvider, ChatRetryOn5xx) {
    TestFixture f;
    f.mock_http->post_responses.push_back({500, "internal server error", {}});
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->text, "Hello world");
    EXPECT_EQ(f.mock_http->post_call_count, 2);
}

// --- Edge Cases ---

TEST(AnthropicProvider, ChatNullInputReturnsEmptyMessages) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    llm::ChatRequest req("claude-sonnet-4-6", nullptr);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    ASSERT_TRUE(body["messages"].is_array());
    EXPECT_TRUE(body["messages"].empty());
}

TEST(AnthropicProvider, ChatConsecutiveToolCallsMerged) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto items = std::make_shared<llm::InputItems>();

    // Two consecutive tool calls → should merge into one assistant message
    auto tc1 = std::make_shared<llm::ToolCallRequest>();
    tc1->call_id = "toolu_1";
    tc1->name = "get_weather";
    tc1->arguments = "{\"city\":\"Berlin\"}";
    items->PushBack(tc1);

    auto tc2 = std::make_shared<llm::ToolCallRequest>();
    tc2->call_id = "toolu_2";
    tc2->name = "get_time";
    tc2->arguments = "{\"tz\":\"CET\"}";
    items->PushBack(tc2);

    llm::ChatRequest req("claude-sonnet-4-6", items);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    ASSERT_TRUE(body["messages"].is_array());
    // Both tool calls merged into a single assistant message
    ASSERT_EQ(body["messages"].size(), 1u);
    EXPECT_EQ(body["messages"][0].value("role", ""), "assistant");
    ASSERT_TRUE(body["messages"][0]["content"].is_array());
    EXPECT_EQ(body["messages"][0]["content"].size(), 2u);
    EXPECT_EQ(body["messages"][0]["content"][0].value("type", ""), "tool_use");
    EXPECT_EQ(body["messages"][0]["content"][1].value("type", ""), "tool_use");
}

TEST(AnthropicProvider, ChatConsecutiveToolResultsMerged) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto items = std::make_shared<llm::InputItems>();

    // Assistant message with two tool calls
    auto tc1 = std::make_shared<llm::ToolCallRequest>();
    tc1->call_id = "toolu_1";
    tc1->name = "get_weather";
    tc1->arguments = "{}";
    items->PushBack(tc1);

    auto tc2 = std::make_shared<llm::ToolCallRequest>();
    tc2->call_id = "toolu_2";
    tc2->name = "get_time";
    tc2->arguments = "{}";
    items->PushBack(tc2);

    // Two consecutive tool results → should merge into one user message
    auto tr1 = std::make_shared<llm::ToolResult>();
    tr1->call_id = "toolu_1";
    tr1->output = json("sunny");
    items->PushBack(tr1);

    auto tr2 = std::make_shared<llm::ToolResult>();
    tr2->call_id = "toolu_2";
    tr2->output = json("14:00");
    items->PushBack(tr2);

    llm::ChatRequest req("claude-sonnet-4-6", items);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    ASSERT_TRUE(body["messages"].is_array());
    // 1 merged assistant message + 1 merged user message
    ASSERT_EQ(body["messages"].size(), 2u);
    EXPECT_EQ(body["messages"][0].value("role", ""), "assistant");
    EXPECT_EQ(body["messages"][0]["content"].size(), 2u);
    EXPECT_EQ(body["messages"][1].value("role", ""), "user");
    EXPECT_EQ(body["messages"][1]["content"].size(), 2u);
}

// --- Coverage: Edge cases in request/response mapping ---

TEST(AnthropicProvider, ChatWithDiscardedToolCallArgs) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto items = std::make_shared<llm::InputItems>();
    auto tcr = std::make_shared<llm::ToolCallRequest>();
    tcr->call_id = "toolu_1";
    tcr->name = "my_tool";
    tcr->arguments = "not valid json {{{";
    items->PushBack(tcr);

    llm::ChatRequest req("claude-sonnet-4-6", items);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    const json& input_obj = body["messages"][0]["content"][0]["input"];
    EXPECT_EQ(input_obj, json::object());
}

TEST(AnthropicProvider, ChatWithStringItemInInputItems) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto items = std::make_shared<llm::InputItems>();
    items->PushBack(std::make_shared<llm::InputString>("hello from user"));

    llm::ChatRequest req("claude-sonnet-4-6", items);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    ASSERT_EQ(body["messages"].size(), 1u);
    EXPECT_EQ(body["messages"][0]["role"], "user");
    EXPECT_EQ(body["messages"][0]["content"][0]["type"], "text");
    EXPECT_EQ(body["messages"][0]["content"][0]["text"], "hello from user");
}

TEST(AnthropicProvider, ChatResponseMissingContent) {
    json j = {{"id", "msg_no_content"},
              {"type", "message"},
              {"role", "assistant"},
              {"stop_reason", "end_turn"},
              {"usage", {{"input_tokens", 5}, {"output_tokens", 0}}}};

    TestFixture f;
    f.mock_http->post_responses.push_back({200, j.dump(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hi");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    EXPECT_EQ(resp, nullptr);
}

TEST(AnthropicProvider, ChatResponseEmptyContent) {
    json j = {{"id", "msg_empty"},        {"type", "message"},
              {"role", "assistant"},      {"stop_reason", "end_turn"},
              {"content", json::array()}, {"usage", {{"input_tokens", 5}, {"output_tokens", 0}}}};

    TestFixture f;
    f.mock_http->post_responses.push_back({200, j.dump(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hi");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    EXPECT_EQ(resp, nullptr);
}

TEST(AnthropicProvider, ChatResponseMultipleTextBlocks) {
    json j = {{"id", "msg_multi"},
              {"type", "message"},
              {"role", "assistant"},
              {"stop_reason", "end_turn"},
              {"content", {{{"type", "text"}, {"text", "Hello "}}, {{"type", "text"}, {"text", "world"}}}},
              {"usage", {{"input_tokens", 5}, {"output_tokens", 10}}}};

    TestFixture f;
    f.mock_http->post_responses.push_back({200, j.dump(), {}});
    AnthropicProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hi");
    llm::ChatRequest req("claude-sonnet-4-6", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->text, "Hello world");
}

}  // namespace
}  // namespace anthropic
}  // namespace llm
}  // namespace foresthub
