#include "foresthub/core/types.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "foresthub/core/input.hpp"
#include "foresthub/core/tools.hpp"
#include "foresthub/util/json.hpp"

using namespace foresthub::core;
using json = nlohmann::json;

// ==========================================
// 1. Tests for ResponseFormat
// ==========================================

TEST(TypesTest, ResponseFormatStruct) {
    ResponseFormat fmt;
    fmt.name = "weather_schema";
    fmt.description = "Schema for weather data";

    json schema = {{"type", "object"}, {"properties", {{"city", {{"type", "string"}}}}}};
    fmt.schema = schema;

    EXPECT_EQ(fmt.name, "weather_schema");
    EXPECT_EQ(fmt.description, "Schema for weather data");

    std::string type_val = fmt.schema["type"];
    EXPECT_EQ(type_val, "object");
}

// ==========================================
// 3. Tests for ChatRequest
// ==========================================

TEST(TypesTest, ChatRequestConstructors) {
    ChatRequest req_default;
    EXPECT_TRUE(req_default.model.empty());
    EXPECT_EQ(req_default.input, nullptr);

    std::string model_name = "gpt-4o";
    auto input_str = std::make_shared<InputString>("Hello");

    ChatRequest req_param(model_name, input_str);

    EXPECT_EQ(req_param.model, "gpt-4o");
    ASSERT_NE(req_param.input, nullptr);

    ASSERT_EQ(req_param.input->GetInputType(), InputType::kString);
    auto casted_input = std::static_pointer_cast<InputString>(req_param.input);
    EXPECT_EQ(casted_input->text, "Hello");
}

TEST(TypesTest, ChatRequestOptionalFields) {
    ChatRequest req;

    req.system_prompt = "You are a helpful assistant";
    EXPECT_EQ(req.system_prompt, "You are a helpful assistant");

    EXPECT_TRUE(req.previous_response_id.empty());
    req.previous_response_id = "resp_12345";
    EXPECT_EQ(req.previous_response_id, "resp_12345");

    EXPECT_FALSE(req.response_format.HasValue());
    ResponseFormat fmt;
    fmt.name = "test";
    req.response_format = fmt;
    EXPECT_TRUE(req.response_format.HasValue());
    EXPECT_EQ(req.response_format->name, "test");
}

TEST(TypesTest, ChatRequestVectors) {
    ChatRequest req;

    auto search_tool = std::make_shared<WebSearch>();
    req.tools.push_back(search_tool);

    ASSERT_EQ(req.tools.size(), 1);
    EXPECT_EQ(req.tools[0]->ToolName(), "web_search");

    req.file_ids.push_back("file_abc");
    req.file_ids.push_back("file_def");
    ASSERT_EQ(req.file_ids.size(), 2);
    EXPECT_EQ(req.file_ids[1], "file_def");

    req.image_urls.push_back("http://example.com/img.png");
    ASSERT_EQ(req.image_urls.size(), 1);
}

TEST(TypesTest, ChatRequestOptionsIntegration) {
    ChatRequest req;

    EXPECT_FALSE(req.options.max_tokens.HasValue());

    req.options.WithMaxTokens(100).WithTemperature(0.5f);

    ASSERT_TRUE(req.options.max_tokens.HasValue());
    EXPECT_EQ(*req.options.max_tokens, 100);
    EXPECT_FLOAT_EQ(*req.options.temperature, 0.5f);
}

// ==========================================
// 4. Tests for ChatResponse
// ==========================================

TEST(TypesTest, ChatResponseStructure) {
    ChatResponse res;
    res.text = "Hello there";
    res.response_id = "id_999";
    res.tokens_used = 42;

    EXPECT_EQ(res.text, "Hello there");
    EXPECT_EQ(res.response_id, "id_999");
    EXPECT_EQ(res.tokens_used, 42);
}

TEST(TypesTest, ChatResponseToolCallRequests) {
    ChatResponse res;

    ToolCallRequest tcr;
    tcr.call_id = "call_1";
    tcr.name = "calculator";
    tcr.arguments = "{}";

    res.tool_call_requests.push_back(tcr);

    ASSERT_EQ(res.tool_call_requests.size(), 1);
    EXPECT_EQ(res.tool_call_requests[0].name, "calculator");
    EXPECT_EQ(res.tool_call_requests[0].call_id, "call_1");
}

TEST(TypesTest, ChatResponseInternalTools) {
    ChatResponse res;

    auto internal_call = std::make_shared<WebSearchToolCall>();
    internal_call->query = "latest news";

    res.tools_called.push_back(internal_call);

    ASSERT_EQ(res.tools_called.size(), 1);
    // Verify content via ToolName() (no RTTI needed)
    EXPECT_EQ(res.tools_called[0]->ToolName(), "web_search");
}

// ==========================================
// 5. Tests for ChatRequest Builder Methods
// ==========================================

TEST(TypesTest, ChatRequestBuilderMethods) {
    auto tool = std::make_shared<WebSearch>();

    ChatRequest req("gpt-4o", std::make_shared<InputString>("Hello"));
    req.WithSystemPrompt("sys")
        .WithPreviousResponseId("prev_1")
        .WithResponseFormat(ResponseFormat{"fmt", {{"type", "object"}}, "desc"})
        .AddTool(tool)
        .AddFileId("file_1")
        .AddImageId("img_1")
        .AddImageUrl("https://example.com/img.png")
        .WithTemperature(0.7f)
        .WithMaxTokens(500)
        .WithTopK(40)
        .WithTopP(0.9f)
        .WithFrequencyPenalty(0.5f)
        .WithPresencePenalty(0.3f)
        .WithSeed(42);

    EXPECT_EQ(req.system_prompt, "sys");
    EXPECT_EQ(req.previous_response_id, "prev_1");
    ASSERT_TRUE(req.response_format.HasValue());
    EXPECT_EQ(req.response_format->name, "fmt");
    ASSERT_EQ(req.tools.size(), 1);
    EXPECT_EQ(req.tools[0]->ToolName(), "web_search");
    ASSERT_EQ(req.file_ids.size(), 1);
    EXPECT_EQ(req.file_ids[0], "file_1");
    ASSERT_EQ(req.image_ids.size(), 1);
    EXPECT_EQ(req.image_ids[0], "img_1");
    ASSERT_EQ(req.image_urls.size(), 1);
    EXPECT_EQ(req.image_urls[0], "https://example.com/img.png");

    ASSERT_TRUE(req.options.temperature.HasValue());
    EXPECT_FLOAT_EQ(*req.options.temperature, 0.7f);
    ASSERT_TRUE(req.options.max_tokens.HasValue());
    EXPECT_EQ(*req.options.max_tokens, 500);
    ASSERT_TRUE(req.options.top_k.HasValue());
    EXPECT_EQ(*req.options.top_k, 40);
    ASSERT_TRUE(req.options.top_p.HasValue());
    EXPECT_FLOAT_EQ(*req.options.top_p, 0.9f);
    ASSERT_TRUE(req.options.frequency_penalty.HasValue());
    EXPECT_FLOAT_EQ(*req.options.frequency_penalty, 0.5f);
    ASSERT_TRUE(req.options.presence_penalty.HasValue());
    EXPECT_FLOAT_EQ(*req.options.presence_penalty, 0.3f);
    ASSERT_TRUE(req.options.seed.HasValue());
    EXPECT_EQ(*req.options.seed, 42);
}
