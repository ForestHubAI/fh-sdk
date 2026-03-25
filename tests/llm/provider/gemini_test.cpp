// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "llm/provider/remote/gemini/provider.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "foresthub/llm/config.hpp"
#include "mocks/mock_http_client.hpp"

namespace foresthub {
namespace provider {
namespace remote {
namespace {

using hal::HttpResponse;
using json = nlohmann::json;

// Helper: create a GeminiProvider with the given MockHttpClient and config.
struct TestFixture {
    std::shared_ptr<tests::MockHttpClient> mock_http = std::make_shared<tests::MockHttpClient>();
    llm::ProviderConfig cfg;

    TestFixture() {
        cfg.api_key = "test-gemini-key";
        cfg.supported_models = {"gemini-2.5-flash", "gemini-2.5-pro"};
    }

    GeminiProvider MakeProvider() { return GeminiProvider(cfg, mock_http); }

    // A valid Gemini generateContent response with text output.
    static std::string TextResponseBody() {
        json j = {
            {"candidates",
             {{{"content", {{"parts", {{{"text", "Hello world"}}}}, {"role", "model"}}}, {"finishReason", "STOP"}}}},
            {"usageMetadata", {{"promptTokenCount", 10}, {"candidatesTokenCount", 32}, {"totalTokenCount", 42}}}};
        return j.dump();
    }

    // A response with a functionCall part.
    static std::string ToolCallResponseBody() {
        json j = {{"candidates",
                   {{{"content",
                      {{"parts", {{{"functionCall", {{"name", "get_weather"}, {"args", {{"city", "Berlin"}}}}}}}},
                       {"role", "model"}}},
                     {"finishReason", "STOP"}}}},
                  {"usageMetadata", {{"promptTokenCount", 8}, {"candidatesTokenCount", 7}, {"totalTokenCount", 15}}}};
        return j.dump();
    }

    // A response with multiple function calls.
    static std::string MultiToolCallResponseBody() {
        json parts = json::array();
        parts.push_back(json{{"functionCall", {{"name", "get_weather"}, {"args", {{"city", "Berlin"}}}}}});
        parts.push_back(json{{"functionCall", {{"name", "get_weather"}, {"args", {{"city", "Madrid"}}}}}});

        json j = {{"candidates", {{{"content", {{"parts", parts}, {"role", "model"}}}, {"finishReason", "STOP"}}}},
                  {"usageMetadata", {{"totalTokenCount", 20}}}};
        return j.dump();
    }

    // A response with Google Search grounding (text with grounding metadata).
    static std::string SearchGroundingResponseBody() {
        json j = {{"candidates",
                   {{{"content", {{"parts", {{{"text", "Berlin has 3.7 million people."}}}}, {"role", "model"}}},
                     {"finishReason", "STOP"},
                     {"groundingMetadata", {{"searchEntryPoint", {{"renderedContent", "<div>...</div>"}}}}}}}},
                  {"usageMetadata", {{"totalTokenCount", 25}}}};
        return j.dump();
    }
};

// --- Construction & Identity ---

TEST(GeminiProvider, Construction) {
    TestFixture f;
    GeminiProvider provider = f.MakeProvider();

    EXPECT_EQ(provider.ProviderId(), "gemini");
}

TEST(GeminiProvider, ConstructionStripsTrailingSlash) {
    TestFixture f;
    f.cfg.base_url = "https://generativelanguage.googleapis.com/";
    GeminiProvider provider = f.MakeProvider();

    f.mock_http->get_responses.push_back({200, "{}", {}});
    provider.Health();
    EXPECT_EQ(f.mock_http->last_url, "https://generativelanguage.googleapis.com/v1beta/models");
}

TEST(GeminiProvider, DefaultBaseUrl) {
    llm::ProviderConfig cfg;
    cfg.api_key = "test-key";
    auto mock = std::make_shared<tests::MockHttpClient>();
    mock->get_responses.push_back({200, "{}", {}});
    GeminiProvider provider(cfg, mock);
    provider.Health();
    EXPECT_EQ(mock->last_url, "https://generativelanguage.googleapis.com/v1beta/models");
}

// --- SupportsModel ---

TEST(GeminiProvider, SupportsModelFound) {
    TestFixture f;
    GeminiProvider provider = f.MakeProvider();

    EXPECT_TRUE(provider.SupportsModel("gemini-2.5-flash"));
    EXPECT_TRUE(provider.SupportsModel("gemini-2.5-pro"));
}

TEST(GeminiProvider, SupportsModelNotFound) {
    TestFixture f;
    GeminiProvider provider = f.MakeProvider();

    EXPECT_FALSE(provider.SupportsModel("gpt-4o"));
}

TEST(GeminiProvider, SupportsModelEmptyListAcceptsAll) {
    TestFixture f;
    f.cfg.supported_models.clear();
    GeminiProvider provider = f.MakeProvider();

    EXPECT_TRUE(provider.SupportsModel("gemini-2.5-flash"));
    EXPECT_TRUE(provider.SupportsModel("any-model"));
}

// --- Health ---

TEST(GeminiProvider, HealthSuccess) {
    TestFixture f;
    f.mock_http->get_responses.push_back({200, "{}", {}});
    GeminiProvider provider = f.MakeProvider();

    std::string err = provider.Health();
    EXPECT_TRUE(err.empty());
}

TEST(GeminiProvider, HealthFailure) {
    TestFixture f;
    f.mock_http->get_responses.push_back({401, "Unauthorized", {}});
    GeminiProvider provider = f.MakeProvider();

    std::string err = provider.Health();
    EXPECT_FALSE(err.empty());
    EXPECT_NE(err.find("401"), std::string::npos);
}

TEST(GeminiProvider, HealthUrlCorrect) {
    TestFixture f;
    f.mock_http->get_responses.push_back({200, "{}", {}});
    GeminiProvider provider = f.MakeProvider();

    provider.Health();
    EXPECT_EQ(f.mock_http->last_url, "https://generativelanguage.googleapis.com/v1beta/models");
}

TEST(GeminiProvider, HealthHeadersContainApiKey) {
    TestFixture f;
    f.mock_http->get_responses.push_back({200, "{}", {}});
    GeminiProvider provider = f.MakeProvider();

    provider.Health();
    EXPECT_EQ(f.mock_http->last_headers["x-goog-api-key"], "test-gemini-key");
    // Must NOT use Bearer auth
    EXPECT_EQ(f.mock_http->last_headers.count("Authorization"), 0u);
}

// --- Chat: Happy Path ---

TEST(GeminiProvider, ChatTextResponse) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("gemini-2.5-flash", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->text, "Hello world");
    EXPECT_EQ(resp->tokens_used, 42);
}

TEST(GeminiProvider, ChatUrlContainsModel) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("gemini-2.5-flash", input);
    provider.Chat(req);

    EXPECT_EQ(f.mock_http->last_url,
              "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent");
}

TEST(GeminiProvider, ChatHeadersCorrect) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("gemini-2.5-flash", input);
    provider.Chat(req);

    EXPECT_EQ(f.mock_http->last_headers["Content-Type"], "application/json");
    EXPECT_EQ(f.mock_http->last_headers["x-goog-api-key"], "test-gemini-key");
}

TEST(GeminiProvider, ChatEmptyResponseId) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("gemini-2.5-flash", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    EXPECT_TRUE(resp->response_id.empty());
}

// --- Chat: Request JSON ---

TEST(GeminiProvider, ChatWithSystemPrompt) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("gemini-2.5-flash", input);
    req.WithSystemPrompt("You are helpful");
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    ASSERT_TRUE(body.contains("systemInstruction"));
    EXPECT_EQ(body["systemInstruction"]["parts"][0]["text"], "You are helpful");
}

TEST(GeminiProvider, ChatWithOptions) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("gemini-2.5-flash", input);
    req.WithTemperature(0.7f).WithMaxTokens(1000).WithTopP(0.9f).WithTopK(40).WithSeed(42);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    ASSERT_TRUE(body.contains("generationConfig"));

    const json& gc = body["generationConfig"];
    EXPECT_FLOAT_EQ(gc.value("temperature", 0.0f), 0.7f);
    EXPECT_EQ(gc.value("maxOutputTokens", 0), 1000);
    EXPECT_FLOAT_EQ(gc.value("topP", 0.0f), 0.9f);
    EXPECT_EQ(gc.value("topK", 0), 40);
    EXPECT_EQ(gc.value("seed", 0), 42);
}

TEST(GeminiProvider, ChatWithPenalties) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("gemini-2.5-flash", input);
    req.WithPresencePenalty(0.5f).WithFrequencyPenalty(0.3f);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    const json& gc = body["generationConfig"];
    EXPECT_FLOAT_EQ(gc.value("presencePenalty", 0.0f), 0.5f);
    EXPECT_FLOAT_EQ(gc.value("frequencyPenalty", 0.0f), 0.3f);
}

TEST(GeminiProvider, ChatWithTools) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Get weather");
    llm::ChatRequest req("gemini-2.5-flash", input);

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
    ASSERT_TRUE(body["tools"][0].contains("functionDeclarations"));
    ASSERT_EQ(body["tools"][0]["functionDeclarations"].size(), 1u);

    const json& decl = body["tools"][0]["functionDeclarations"][0];
    EXPECT_EQ(decl.value("name", ""), "get_weather");
    EXPECT_EQ(decl.value("description", ""), "Get the weather");
    EXPECT_TRUE(decl.contains("parameters"));
}

TEST(GeminiProvider, ChatWithResponseFormat) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Give me JSON");
    llm::ChatRequest req("gemini-2.5-flash", input);
    llm::ResponseFormat fmt;
    fmt.name = "my_schema";
    fmt.schema = json{{"type", "object"}, {"properties", {{"answer", {{"type", "string"}}}}}};
    req.WithResponseFormat(fmt);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    ASSERT_TRUE(body.contains("generationConfig"));

    const json& gc = body["generationConfig"];
    EXPECT_EQ(gc.value("responseMimeType", ""), "application/json");
    ASSERT_TRUE(gc.contains("responseJsonSchema"));
    // Gemini expects the JSON Schema directly (not wrapped in a name/schema object)
    EXPECT_EQ(gc["responseJsonSchema"].value("type", ""), "object");
    EXPECT_TRUE(gc["responseJsonSchema"].contains("properties"));
}

TEST(GeminiProvider, ChatToolSchemaStrictified) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Get weather");
    llm::ChatRequest req("gemini-2.5-flash", input);

    // Minimal schema with additionalProperties (should be stripped)
    auto fn_tool = std::make_shared<llm::FunctionTool>();
    fn_tool->name = "get_weather";
    fn_tool->description = "Get the weather";
    fn_tool->parameters = json{{"city", {{"type", "string"}}}, {"units", {{"type", "string"}}}};
    req.AddTool(fn_tool);

    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    const json& params = body["tools"][0]["functionDeclarations"][0]["parameters"];

    // NormalizeSchema: wrapped into full format
    EXPECT_EQ(params.value("type", ""), "object");
    EXPECT_TRUE(params.contains("properties"));
    // EnsureAllRequired: all keys in required
    ASSERT_TRUE(params.contains("required"));
    EXPECT_EQ(params["required"].size(), 2u);
    // StripAdditionalProperties: no additionalProperties
    EXPECT_FALSE(params.contains("additionalProperties"));
}

TEST(GeminiProvider, ChatToolSchemaStripsUserAdditionalProperties) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Get weather");
    llm::ChatRequest req("gemini-2.5-flash", input);

    // User-provided additionalProperties:false should be stripped for Gemini
    auto fn_tool = std::make_shared<llm::FunctionTool>();
    fn_tool->name = "get_weather";
    fn_tool->description = "Get the weather";
    fn_tool->parameters =
        json{{"type", "object"}, {"properties", {{"city", {{"type", "string"}}}}}, {"additionalProperties", false}};
    req.AddTool(fn_tool);

    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    const json& params = body["tools"][0]["functionDeclarations"][0]["parameters"];

    EXPECT_FALSE(params.contains("additionalProperties"));
}

TEST(GeminiProvider, ChatResponseFormatSchemaStrictified) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Give me JSON");
    llm::ChatRequest req("gemini-2.5-flash", input);
    llm::ResponseFormat fmt;
    fmt.name = "test";
    fmt.schema = json{{"type", "object"}, {"properties", {{"answer", {{"type", "string"}}}}}};
    req.WithResponseFormat(fmt);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    const json& schema = body["generationConfig"]["responseJsonSchema"];

    ASSERT_TRUE(schema.contains("required"));
    EXPECT_EQ(schema["required"].size(), 1u);
    EXPECT_FALSE(schema.contains("additionalProperties"));
}

TEST(GeminiProvider, ChatToolResultInput) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto items = std::make_shared<llm::InputItems>();

    auto tcr = std::make_shared<llm::ToolCallRequest>();
    tcr->call_id = "gemini_call_0";
    tcr->name = "get_weather";
    tcr->arguments = "{\"city\":\"Berlin\"}";
    items->PushBack(tcr);

    auto tr = std::make_shared<llm::ToolResult>();
    tr->call_id = "gemini_call_0";
    tr->name = "get_weather";
    tr->output = json{{"city", "Berlin"}, {"temp", "15C"}};
    items->PushBack(tr);

    llm::ChatRequest req("gemini-2.5-flash", items);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    ASSERT_TRUE(body.contains("contents"));
    ASSERT_EQ(body["contents"].size(), 2u);

    // Tool call → role: "model", functionCall
    EXPECT_EQ(body["contents"][0].value("role", ""), "model");
    ASSERT_TRUE(body["contents"][0]["parts"][0].contains("functionCall"));
    EXPECT_EQ(body["contents"][0]["parts"][0]["functionCall"]["name"], "get_weather");

    // Tool result → role: "user", functionResponse
    EXPECT_EQ(body["contents"][1].value("role", ""), "user");
    ASSERT_TRUE(body["contents"][1]["parts"][0].contains("functionResponse"));
    EXPECT_EQ(body["contents"][1]["parts"][0]["functionResponse"]["name"], "get_weather");
}

// --- Chat: Response Parsing ---

TEST(GeminiProvider, ChatToolCallResponse) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::ToolCallResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("What is the weather?");
    llm::ChatRequest req("gemini-2.5-flash", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    ASSERT_EQ(resp->tool_call_requests.size(), 1u);
    EXPECT_EQ(resp->tool_call_requests[0].call_id, "gemini_call_0");
    EXPECT_EQ(resp->tool_call_requests[0].name, "get_weather");
    // Args are stringified JSON (not a raw object)
    json args = json::parse(resp->tool_call_requests[0].arguments, nullptr, false);
    ASSERT_FALSE(args.is_discarded());
    EXPECT_EQ(args.value("city", ""), "Berlin");
}

TEST(GeminiProvider, ChatMultipleToolCalls) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::MultiToolCallResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Weather in Berlin and Madrid?");
    llm::ChatRequest req("gemini-2.5-flash", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    ASSERT_EQ(resp->tool_call_requests.size(), 2u);
    EXPECT_EQ(resp->tool_call_requests[0].call_id, "gemini_call_0");
    EXPECT_EQ(resp->tool_call_requests[1].call_id, "gemini_call_1");
    EXPECT_EQ(resp->tool_call_requests[0].name, "get_weather");
    EXPECT_EQ(resp->tool_call_requests[1].name, "get_weather");
}

TEST(GeminiProvider, ChatTokenUsage) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("gemini-2.5-flash", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->tokens_used, 42);
}

// --- Chat: Google Search ---

TEST(GeminiProvider, ChatWithSearchGrounding) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::SearchGroundingResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Population of Berlin?");
    llm::ChatRequest req("gemini-2.5-flash", input);
    req.AddTool(std::make_shared<llm::WebSearch>());
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    ASSERT_TRUE(body.contains("tools"));
    ASSERT_EQ(body["tools"].size(), 1u);
    EXPECT_TRUE(body["tools"][0].contains("google_search"));
}

TEST(GeminiProvider, ChatSearchDroppedWhenToolsPresent) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Get weather and search");
    llm::ChatRequest req("gemini-2.5-flash", input);

    // Add function tool AND web search
    auto fn_tool = std::make_shared<llm::FunctionTool>();
    fn_tool->name = "get_weather";
    fn_tool->description = "Get the weather";
    fn_tool->parameters = json{{"type", "object"}};
    req.AddTool(fn_tool);
    req.AddTool(std::make_shared<llm::WebSearch>());

    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    ASSERT_TRUE(body.contains("tools"));

    // Only functionDeclarations, no google_search
    ASSERT_EQ(body["tools"].size(), 1u);
    EXPECT_TRUE(body["tools"][0].contains("functionDeclarations"));
    EXPECT_FALSE(body["tools"][0].contains("google_search"));
}

// --- Chat: Error Paths ---

TEST(GeminiProvider, ChatHttpError) {
    TestFixture f;
    f.mock_http->post_responses.push_back({400, "bad request", {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("gemini-2.5-flash", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    EXPECT_EQ(resp, nullptr);
}

TEST(GeminiProvider, ChatMalformedJson) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, "not valid json {{{", {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("gemini-2.5-flash", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    EXPECT_EQ(resp, nullptr);
}

TEST(GeminiProvider, ChatEmptyCandidates) {
    TestFixture f;
    json j = {{"candidates", json::array()}};
    f.mock_http->post_responses.push_back({200, j.dump(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("gemini-2.5-flash", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    EXPECT_EQ(resp, nullptr);
}

// --- Retry Logic ---

TEST(GeminiProvider, ChatRetryOn429) {
    TestFixture f;
    f.mock_http->post_responses.push_back({429, "rate limited", {}});
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("gemini-2.5-flash", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->text, "Hello world");
    EXPECT_EQ(f.mock_http->post_call_count, 2);
}

TEST(GeminiProvider, ChatNoRetryOn4xx) {
    TestFixture f;
    f.mock_http->post_responses.push_back({403, "forbidden", {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hello");
    llm::ChatRequest req("gemini-2.5-flash", input);
    provider.Chat(req);

    EXPECT_EQ(f.mock_http->post_call_count, 1);
}

// --- Coverage: Edge cases in request/response mapping ---

TEST(GeminiProvider, ChatWithNullInput) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    llm::ChatRequest req("gemini-2.5-flash", nullptr);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    ASSERT_TRUE(body["contents"].is_array());
    EXPECT_TRUE(body["contents"].empty());
}

TEST(GeminiProvider, ChatWithDiscardedToolCallArgs) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto items = std::make_shared<llm::InputItems>();
    auto tcr = std::make_shared<llm::ToolCallRequest>();
    tcr->call_id = "c1";
    tcr->name = "my_tool";
    tcr->arguments = "not valid json {{{";
    items->PushBack(tcr);

    llm::ChatRequest req("gemini-2.5-flash", items);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    const json& fc = body["contents"][0]["parts"][0]["functionCall"];
    EXPECT_EQ(fc["args"], json::object());
}

TEST(GeminiProvider, ChatWithStringToolResult) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto items = std::make_shared<llm::InputItems>();
    auto tr = std::make_shared<llm::ToolResult>();
    tr->call_id = "c1";
    tr->name = "my_tool";
    tr->output = json("a plain string");
    items->PushBack(tr);

    llm::ChatRequest req("gemini-2.5-flash", items);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    const json& resp_obj = body["contents"][0]["parts"][0]["functionResponse"]["response"];
    EXPECT_EQ(resp_obj["result"], "a plain string");
}

TEST(GeminiProvider, ChatWithNonObjectToolResult) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto items = std::make_shared<llm::InputItems>();
    auto tr = std::make_shared<llm::ToolResult>();
    tr->call_id = "c1";
    tr->name = "my_tool";
    tr->output = json(42);
    items->PushBack(tr);

    llm::ChatRequest req("gemini-2.5-flash", items);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    const json& resp_obj = body["contents"][0]["parts"][0]["functionResponse"]["response"];
    EXPECT_EQ(resp_obj["result"], "42");
}

TEST(GeminiProvider, ChatWithStringItemInInputItems) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto items = std::make_shared<llm::InputItems>();
    items->PushBack(std::make_shared<llm::InputString>("hello from user"));

    llm::ChatRequest req("gemini-2.5-flash", items);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    EXPECT_EQ(body["contents"][0]["role"], "user");
    EXPECT_EQ(body["contents"][0]["parts"][0]["text"], "hello from user");
}

TEST(GeminiProvider, ChatResponseMultipleTextParts) {
    json j = {{"candidates",
               {{{"content", {{"parts", {{{"text", "Hello "}}, {{"text", "world"}}}}, {"role", "model"}}},
                 {"finishReason", "STOP"}}}},
              {"usageMetadata", {{"totalTokenCount", 10}}}};

    TestFixture f;
    f.mock_http->post_responses.push_back({200, j.dump(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hi");
    llm::ChatRequest req("gemini-2.5-flash", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->text, "Hello world");
}

TEST(GeminiProvider, ChatToolCallWithoutArgs) {
    json j = {{"candidates",
               {{{"content", {{"parts", {{{"functionCall", {{"name", "no_args_tool"}}}}}}, {"role", "model"}}},
                 {"finishReason", "STOP"}}}},
              {"usageMetadata", {{"totalTokenCount", 5}}}};

    TestFixture f;
    f.mock_http->post_responses.push_back({200, j.dump(), {}});
    GeminiProvider provider = f.MakeProvider();

    auto input = std::make_shared<llm::InputString>("Hi");
    llm::ChatRequest req("gemini-2.5-flash", input);
    std::shared_ptr<llm::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    ASSERT_EQ(resp->tool_call_requests.size(), 1u);
    EXPECT_EQ(resp->tool_call_requests[0].arguments, "{}");
}

}  // namespace
}  // namespace remote
}  // namespace provider
}  // namespace foresthub
