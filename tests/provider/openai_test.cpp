#include "foresthub/provider/remote/openai.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "foresthub/config/config.hpp"
#include "mocks/mock_http_client.hpp"

namespace foresthub {
namespace provider {
namespace remote {
namespace {

using core::HttpResponse;
using json = nlohmann::json;

// Helper: create an OpenAIProvider with the given MockHttpClient and config.
struct TestFixture {
    std::shared_ptr<tests::MockHttpClient> mock_http = std::make_shared<tests::MockHttpClient>();
    config::ProviderConfig cfg;

    TestFixture() {
        cfg.api_key = "sk-test-key-123";
        // base_url left empty — provider uses its default (https://api.openai.com)
        cfg.supported_models = {"gpt-4o", "gpt-4o-mini"};
    }

    OpenAIProvider MakeProvider() { return OpenAIProvider(cfg, mock_http); }

    // A valid OpenAI Responses API response body with text output.
    static std::string TextResponseBody() {
        json j = {
            {"id", "resp_abc123"},
            {"output", {{{"type", "message"}, {"content", {{{"type", "output_text"}, {"text", "Hello world"}}}}}}},
            {"usage", {{"total_tokens", 42}, {"input_tokens", 10}, {"output_tokens", 32}}}};
        return j.dump();
    }

    // A response body with a function_call output item.
    static std::string ToolCallResponseBody() {
        json j = {{"id", "resp_tool1"},
                  {"output",
                   {{{"type", "function_call"},
                     {"call_id", "call_abc"},
                     {"name", "get_weather"},
                     {"arguments", "{\"city\":\"Berlin\"}"}}}},
                  {"usage", {{"total_tokens", 15}}}};
        return j.dump();
    }

    // A response body with a web_search_call output item including queries.
    static std::string WebSearchResponseBody() {
        json j = {{"id", "resp_ws1"},
                  {"output",
                   {{{"type", "web_search_call"},
                     {"id", "ws_123"},
                     {"status", "completed"},
                     {"action", "search"},
                     {"queries", json::array({"ESP32 deep sleep power consumption"})}}}},
                  {"usage", {{"total_tokens", 20}}}};
        return j.dump();
    }
};

// --- Construction & Identity ---

TEST(OpenAIProvider, Construction) {
    TestFixture f;
    OpenAIProvider provider = f.MakeProvider();

    EXPECT_EQ(provider.ProviderId(), "openai");
}

TEST(OpenAIProvider, ConstructionStripsTrailingSlash) {
    TestFixture f;
    f.cfg.base_url = "https://api.openai.com/";
    OpenAIProvider provider = f.MakeProvider();

    // Verify via Health URL (no double slash)
    f.mock_http->get_responses.push_back({200, "{}", {}});
    provider.Health();
    EXPECT_EQ(f.mock_http->last_url, "https://api.openai.com/v1/models");
}

TEST(OpenAIProvider, DefaultBaseUrl) {
    config::ProviderConfig cfg;
    cfg.api_key = "sk-test";
    // base_url left empty — provider applies its default
    auto mock = std::make_shared<tests::MockHttpClient>();
    mock->get_responses.push_back({200, "{}", {}});
    OpenAIProvider provider(cfg, mock);
    provider.Health();
    EXPECT_EQ(mock->last_url, "https://api.openai.com/v1/models");
}

// --- SupportsModel ---

TEST(OpenAIProvider, SupportsModelFound) {
    TestFixture f;
    OpenAIProvider provider = f.MakeProvider();

    EXPECT_TRUE(provider.SupportsModel("gpt-4o"));
    EXPECT_TRUE(provider.SupportsModel("gpt-4o-mini"));
}

TEST(OpenAIProvider, SupportsModelNotFound) {
    TestFixture f;
    OpenAIProvider provider = f.MakeProvider();

    EXPECT_FALSE(provider.SupportsModel("claude-3"));
}

TEST(OpenAIProvider, SupportsModelEmptyListAcceptsAll) {
    TestFixture f;
    f.cfg.supported_models.clear();
    OpenAIProvider provider = f.MakeProvider();

    EXPECT_TRUE(provider.SupportsModel("gpt-4o"));
    EXPECT_TRUE(provider.SupportsModel("any-model"));
}

// --- Health ---

TEST(OpenAIProvider, HealthSuccess) {
    TestFixture f;
    f.mock_http->get_responses.push_back({200, "{}", {}});
    OpenAIProvider provider = f.MakeProvider();

    std::string err = provider.Health();
    EXPECT_TRUE(err.empty());
    EXPECT_EQ(f.mock_http->last_url, "https://api.openai.com/v1/models");
}

TEST(OpenAIProvider, HealthFailure) {
    TestFixture f;
    f.mock_http->get_responses.push_back({401, "Unauthorized", {}});
    OpenAIProvider provider = f.MakeProvider();

    std::string err = provider.Health();
    EXPECT_FALSE(err.empty());
    EXPECT_NE(err.find("401"), std::string::npos);
}

TEST(OpenAIProvider, HealthHeadersContainBearer) {
    TestFixture f;
    f.mock_http->get_responses.push_back({200, "{}", {}});
    OpenAIProvider provider = f.MakeProvider();

    provider.Health();
    EXPECT_EQ(f.mock_http->last_headers["Authorization"], "Bearer sk-test-key-123");
}

// --- Chat: Happy Path ---

TEST(OpenAIProvider, ChatTextResponse) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    OpenAIProvider provider = f.MakeProvider();

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("gpt-4o", input);
    std::shared_ptr<core::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->text, "Hello world");
    EXPECT_EQ(resp->response_id, "resp_abc123");
    EXPECT_EQ(resp->tokens_used, 42);
}

TEST(OpenAIProvider, ChatUrlCorrect) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    OpenAIProvider provider = f.MakeProvider();

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("gpt-4o", input);
    provider.Chat(req);

    EXPECT_EQ(f.mock_http->last_url, "https://api.openai.com/v1/responses");
}

TEST(OpenAIProvider, ChatHeadersCorrect) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    OpenAIProvider provider = f.MakeProvider();

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("gpt-4o", input);
    provider.Chat(req);

    EXPECT_EQ(f.mock_http->last_headers["Content-Type"], "application/json");
    EXPECT_EQ(f.mock_http->last_headers["Authorization"], "Bearer sk-test-key-123");
}

// --- Chat: Request JSON Verification ---

TEST(OpenAIProvider, ChatWithSystemPrompt) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    OpenAIProvider provider = f.MakeProvider();

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("gpt-4o", input);
    req.WithSystemPrompt("You are helpful");
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    EXPECT_EQ(body.value("instructions", ""), "You are helpful");
}

TEST(OpenAIProvider, ChatWithPreviousResponseId) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    OpenAIProvider provider = f.MakeProvider();

    auto input = std::make_shared<core::InputString>("Continue");
    core::ChatRequest req("gpt-4o", input);
    req.WithPreviousResponseId("resp_prev_123");
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    EXPECT_EQ(body.value("previous_response_id", ""), "resp_prev_123");
}

TEST(OpenAIProvider, ChatWithOptions) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    OpenAIProvider provider = f.MakeProvider();

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("gpt-4o", input);
    req.WithTemperature(0.7f).WithMaxTokens(1000).WithTopP(0.9f);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    EXPECT_EQ(body.value("max_output_tokens", 0), 1000);
    EXPECT_FLOAT_EQ(body.value("temperature", 0.0f), 0.7f);
    EXPECT_FLOAT_EQ(body.value("top_p", 0.0f), 0.9f);
}

TEST(OpenAIProvider, ChatWithTools) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    OpenAIProvider provider = f.MakeProvider();

    auto input = std::make_shared<core::InputString>("Search for weather");
    core::ChatRequest req("gpt-4o", input);

    // Add a function tool
    auto fn_tool = std::make_shared<core::FunctionTool>();
    fn_tool->name = "get_weather";
    fn_tool->description = "Get the weather";
    fn_tool->parameters = json{{"type", "object"}, {"properties", {{"city", {{"type", "string"}}}}}};
    req.AddTool(fn_tool);

    // Add web search
    req.AddTool(std::make_shared<core::WebSearch>());

    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    ASSERT_TRUE(body.contains("tools"));
    ASSERT_EQ(body["tools"].size(), 2u);

    // Function tool: flat format (Responses API)
    EXPECT_EQ(body["tools"][0].value("type", ""), "function");
    EXPECT_EQ(body["tools"][0].value("name", ""), "get_weather");
    EXPECT_EQ(body["tools"][0].value("description", ""), "Get the weather");
    EXPECT_TRUE(body["tools"][0].contains("parameters"));

    // Web search tool
    EXPECT_EQ(body["tools"][1].value("type", ""), "web_search");
}

TEST(OpenAIProvider, ChatWithResponseFormat) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    OpenAIProvider provider = f.MakeProvider();

    auto input = std::make_shared<core::InputString>("Give me JSON");
    core::ChatRequest req("gpt-4o", input);
    core::ResponseFormat fmt;
    fmt.name = "my_schema";
    fmt.schema = json{{"type", "object"}, {"properties", {{"answer", {{"type", "string"}}}}}};
    fmt.description = "A test schema";
    req.WithResponseFormat(fmt);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    ASSERT_TRUE(body.contains("text"));
    ASSERT_TRUE(body["text"].contains("format"));

    const json& format = body["text"]["format"];
    EXPECT_EQ(format.value("type", ""), "json_schema");
    EXPECT_EQ(format.value("name", ""), "my_schema");
    EXPECT_EQ(format.value("strict", false), true);
    EXPECT_EQ(format.value("description", ""), "A test schema");
    EXPECT_TRUE(format.contains("schema"));
}

TEST(OpenAIProvider, ChatToolResultInput) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    OpenAIProvider provider = f.MakeProvider();

    // Build InputItems with a ToolCallRequest and ToolResult
    auto items = std::make_shared<core::InputItems>();

    auto tcr = std::make_shared<core::ToolCallRequest>();
    tcr->call_id = "call_1";
    tcr->name = "get_weather";
    tcr->arguments = "{\"city\":\"Berlin\"}";
    items->PushBack(tcr);

    auto tr = std::make_shared<core::ToolResult>();
    tr->call_id = "call_1";
    tr->name = "get_weather";
    tr->output = json("sunny, 22C");
    items->PushBack(tr);

    core::ChatRequest req("gpt-4o", items);
    provider.Chat(req);

    json body = json::parse(f.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    ASSERT_TRUE(body["input"].is_array());
    ASSERT_EQ(body["input"].size(), 2u);

    EXPECT_EQ(body["input"][0].value("type", ""), "function_call");
    EXPECT_EQ(body["input"][0].value("call_id", ""), "call_1");
    EXPECT_EQ(body["input"][0].value("name", ""), "get_weather");

    EXPECT_EQ(body["input"][1].value("type", ""), "function_call_output");
    EXPECT_EQ(body["input"][1].value("call_id", ""), "call_1");
    EXPECT_EQ(body["input"][1].value("output", ""), "sunny, 22C");
}

// --- Chat: Response Parsing ---

TEST(OpenAIProvider, ChatToolCallResponse) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::ToolCallResponseBody(), {}});
    OpenAIProvider provider = f.MakeProvider();

    auto input = std::make_shared<core::InputString>("What is the weather?");
    core::ChatRequest req("gpt-4o", input);
    std::shared_ptr<core::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    ASSERT_EQ(resp->tool_call_requests.size(), 1u);
    EXPECT_EQ(resp->tool_call_requests[0].call_id, "call_abc");
    EXPECT_EQ(resp->tool_call_requests[0].name, "get_weather");
    EXPECT_EQ(resp->tool_call_requests[0].arguments, "{\"city\":\"Berlin\"}");
    EXPECT_EQ(resp->response_id, "resp_tool1");
}

TEST(OpenAIProvider, ChatWebSearchResponse) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, TestFixture::WebSearchResponseBody(), {}});
    OpenAIProvider provider = f.MakeProvider();

    auto input = std::make_shared<core::InputString>("Search the web");
    core::ChatRequest req("gpt-4o", input);
    std::shared_ptr<core::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    ASSERT_EQ(resp->tools_called.size(), 1u);
    EXPECT_EQ(resp->tools_called[0]->ToolName(), "web_search");
    auto ws = std::static_pointer_cast<core::WebSearchToolCall>(resp->tools_called[0]);
    EXPECT_EQ(ws->query, "ESP32 deep sleep power consumption");
}

// --- Chat: Error Paths ---

TEST(OpenAIProvider, ChatHttpError) {
    TestFixture f;
    f.mock_http->post_responses.push_back({400, "bad request", {}});
    OpenAIProvider provider = f.MakeProvider();

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("gpt-4o", input);
    std::shared_ptr<core::ChatResponse> resp = provider.Chat(req);

    EXPECT_EQ(resp, nullptr);
}

TEST(OpenAIProvider, ChatMalformedJson) {
    TestFixture f;
    f.mock_http->post_responses.push_back({200, "not valid json {{{", {}});
    OpenAIProvider provider = f.MakeProvider();

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("gpt-4o", input);
    std::shared_ptr<core::ChatResponse> resp = provider.Chat(req);

    EXPECT_EQ(resp, nullptr);
}

// --- Retry Logic ---

TEST(OpenAIProvider, ChatRetryOn429) {
    TestFixture f;
    f.mock_http->post_responses.push_back({429, "rate limited", {}});
    f.mock_http->post_responses.push_back({200, TestFixture::TextResponseBody(), {}});
    OpenAIProvider provider = f.MakeProvider();

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("gpt-4o", input);
    std::shared_ptr<core::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->text, "Hello world");
    EXPECT_EQ(f.mock_http->post_call_count, 2);
}

TEST(OpenAIProvider, ChatNoRetryOn4xx) {
    TestFixture f;
    f.mock_http->post_responses.push_back({403, "forbidden", {}});
    OpenAIProvider provider = f.MakeProvider();

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("gpt-4o", input);
    provider.Chat(req);

    EXPECT_EQ(f.mock_http->post_call_count, 1);
}

}  // namespace
}  // namespace remote
}  // namespace provider
}  // namespace foresthub
