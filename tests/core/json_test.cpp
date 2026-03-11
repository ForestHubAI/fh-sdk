#include "foresthub/util/json.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "foresthub/core/input.hpp"
#include "foresthub/core/options.hpp"
#include "foresthub/core/serialization.hpp"
#include "foresthub/core/tools.hpp"
#include "foresthub/core/types.hpp"

using namespace foresthub::core;
using json = nlohmann::json;

// ==========================================
// 1. Tests for Options Serialization
// ==========================================

TEST(JsonTest, SerializeOptions) {
    Options opts;
    opts.WithMaxTokens(100).WithTemperature(0.5f);

    json j = opts;

    EXPECT_EQ(j["maxTokens"], 100);
    EXPECT_EQ(j["temperature"], 0.5f);

    // Verify unset fields are not in JSON
    EXPECT_FALSE(j.contains("topP"));
    EXPECT_FALSE(j.contains("seed"));
}

TEST(JsonTest, DeserializeOptions) {
    json j = {{"maxTokens", 256}, {"topP", 0.9}};

    Options opts = j.get<Options>();

    ASSERT_TRUE(opts.max_tokens.HasValue());
    EXPECT_EQ(*opts.max_tokens, 256);

    ASSERT_TRUE(opts.top_p.HasValue());
    EXPECT_FLOAT_EQ(*opts.top_p, 0.9f);

    // Check missing fields remain empty
    EXPECT_FALSE(opts.temperature.HasValue());
}

// ==========================================
// 2. Tests for ResponseFormat
// ==========================================

TEST(JsonTest, SerializeResponseFormat) {
    ResponseFormat fmt;
    fmt.name = "weather";
    fmt.description = "Weather info";
    fmt.schema = {{"type", "object"}};

    json j = fmt;

    EXPECT_EQ(j["name"], "weather");
    EXPECT_EQ(j["description"], "Weather info");
    EXPECT_EQ(j["schema"]["type"], "object");
}

TEST(JsonTest, DeserializeResponseFormat) {
    json j = {{"name", "math"}, {"schema", {{"type", "number"}}}};

    ResponseFormat fmt = j.get<ResponseFormat>();

    EXPECT_EQ(fmt.name, "math");
    EXPECT_EQ(fmt.schema["type"], "number");
    // Description was omitted in JSON, should be empty string
    EXPECT_TRUE(fmt.description.empty());
}

// ==========================================
// 3. Tests for InputItem Polymorphism
// ==========================================

TEST(JsonTest, SerializeInputStringViaPointer) {
    std::shared_ptr<InputItem> item = std::make_shared<InputString>("Hello World");

    json j;
    foresthub::core::to_json(j, item);

    EXPECT_EQ(j["value"], "Hello World");
}

TEST(JsonTest, SerializeToolCallRequestViaPointer) {
    auto req = std::make_shared<ToolCallRequest>();
    req->call_id = "call_1";
    req->name = "func";
    req->arguments = "{\"a\": 1}";

    std::shared_ptr<InputItem> item = req;
    json j;
    foresthub::core::to_json(j, item);  // Also possible json j = item;

    EXPECT_EQ(j["callId"], "call_1");
    EXPECT_EQ(j["name"], "func");
    // The implementation tries to parse arguments string as JSON object
    EXPECT_EQ(j["arguments"]["a"], 1);
}

TEST(JsonTest, SerializeToolResultViaPointer) {
    auto res = std::make_shared<ToolResult>();
    res->call_id = "call_1";
    res->name = "func";
    res->output = json("result_string");

    std::shared_ptr<InputItem> item = res;
    json j;
    foresthub::core::to_json(j, item);

    EXPECT_EQ(j["callId"], "call_1");
    EXPECT_EQ(j["output"], "result_string");
}

// ==========================================
// 4. Tests for Tools Polymorphism
// ==========================================

TEST(JsonTest, SerializeFunctionToolViaPointer) {
    auto ft = std::make_shared<FunctionTool>();
    ft->name = "my_tool";
    ft->description = "Does stuff";
    ft->parameters = {{"type", "object"}};

    std::shared_ptr<Tool> tool = ft;

    json j;
    foresthub::core::to_json(j, tool);

    EXPECT_EQ(j["name"], "my_tool");
    EXPECT_EQ(j["description"], "Does stuff");
    EXPECT_EQ(j["parameters"]["type"], "object");
}

TEST(JsonTest, SerializeWebSearchToolViaPointer) {
    auto ws = std::make_shared<WebSearch>();

    std::shared_ptr<Tool> tool = ws;

    json j;
    foresthub::core::to_json(j, tool);

    EXPECT_EQ(j["type"], "web_search");
}

// ==========================================
// 5. Tests for ChatRequest (Complex Object)
// ==========================================

TEST(JsonTest, SerializeChatRequest_InputString) {
    ChatRequest req;
    req.model = "gpt-4o";
    req.input = std::make_shared<InputString>("User input");

    json j = req;

    EXPECT_EQ(j["model"], "gpt-4o");
    EXPECT_EQ(j["input"]["value"], "User input");
}

TEST(JsonTest, SerializeChatRequest_InputItems) {
    ChatRequest req;
    req.model = "gpt-4o";

    auto list = std::make_shared<InputItems>();
    list->PushBack(std::make_shared<InputString>("Hello"));

    auto tr = std::make_shared<ToolResult>();
    tr->call_id = "call_1";
    tr->name = "func";
    tr->output = json("result");
    list->PushBack(tr);

    req.input = list;

    json j = req;

    ASSERT_TRUE(j["input"].is_array());
    EXPECT_EQ(j["input"].size(), 2);
    EXPECT_EQ(j["input"][0]["value"], "Hello");
    EXPECT_EQ(j["input"][1]["callId"], "call_1");
    EXPECT_EQ(j["input"][1]["output"], "result");
}

TEST(JsonTest, DeserializeChatRequest_InputString) {
    json j = {{"model", "mistral"}, {"input", {{"value", "Hi"}}}, {"options", {{"temperature", 0.7}}}};

    ChatRequest req = j.get<ChatRequest>();

    EXPECT_EQ(req.model, "mistral");
    ASSERT_NE(req.input, nullptr);

    ASSERT_EQ(req.input->GetInputType(), InputType::kString);
    auto input_str = std::static_pointer_cast<InputString>(req.input);
    EXPECT_EQ(input_str->text, "Hi");

    ASSERT_TRUE(req.options.temperature.HasValue());
    EXPECT_FLOAT_EQ(*req.options.temperature, 0.7f);
}

TEST(JsonTest, DeserializeChatRequest_InputArray) {
    json j = {{"model", "gpt-4"}, {"input", json::array({{{"value", "Msg 1"}}, {{"value", "Msg 2"}}})}};

    ChatRequest req = j.get<ChatRequest>();

    ASSERT_EQ(req.input->GetInputType(), InputType::kItems);
    auto input_list = std::static_pointer_cast<InputItems>(req.input);
    ASSERT_EQ(input_list->items.size(), 2);
    EXPECT_EQ(input_list->items[0]->ToString(), "Msg 1");
}

// ==========================================
// 6. Tests for ChatResponse
// ==========================================

TEST(JsonTest, DeserializeChatResponse) {
    json j = {{"text", "This is a Test"},
              {"responseID", "id_123"},
              {"tokensUsed", 50},
              {"toolCallRequests", {{{"callId", "call_abc"}, {"name", "calc"}, {"arguments", "{\"a\":1}"}}}}};

    ChatResponse res = j.get<ChatResponse>();

    EXPECT_EQ(res.text, "This is a Test");
    EXPECT_EQ(res.response_id, "id_123");
    EXPECT_EQ(res.tokens_used, 50);

    ASSERT_EQ(res.tool_call_requests.size(), 1);
    EXPECT_EQ(res.tool_call_requests[0].name, "calc");
    // Note: arguments are stored as raw string in the struct
    EXPECT_EQ(res.tool_call_requests[0].arguments, "{\"a\":1}");
}

// ==========================================
// 7. Tests for Missing Serialization Branches
// ==========================================

TEST(JsonTest, SerializeNullInputItemPointer) {
    std::shared_ptr<InputItem> null_item = nullptr;

    json j;
    foresthub::core::to_json(j, null_item);

    EXPECT_TRUE(j.is_null());
}

TEST(JsonTest, SerializeNullToolPointer) {
    std::shared_ptr<Tool> null_tool = nullptr;

    json j;
    foresthub::core::to_json(j, null_tool);

    EXPECT_TRUE(j.is_null());
}

TEST(JsonTest, SerializeChatRequest_AllFields) {
    ChatRequest req("gpt-4o", std::make_shared<InputString>("Hi"));
    req.WithSystemPrompt("sys")
        .WithPreviousResponseId("prev_1")
        .AddFileId("f1")
        .AddFileId("f2")
        .AddImageId("i1")
        .AddImageUrl("https://example.com/img.png");

    json j = req;

    EXPECT_EQ(j["previousResponseID"], "prev_1");
    ASSERT_EQ(j["fileIDs"].size(), 2);
    EXPECT_EQ(j["fileIDs"][0], "f1");
    ASSERT_EQ(j["imageIDs"].size(), 1);
    EXPECT_EQ(j["imageIDs"][0], "i1");
    ASSERT_EQ(j["imageURLs"].size(), 1);
    EXPECT_EQ(j["imageURLs"][0], "https://example.com/img.png");
}

TEST(JsonTest, DeserializeChatRequest_ToolCallRequestInArray) {
    json j = {{"model", "gpt-4o"},
              {"input", json::array({{{"callId", "call_1"}, {"name", "func"}, {"arguments", R"({"a":1})"}}})}};

    ChatRequest req = j.get<ChatRequest>();

    ASSERT_NE(req.input, nullptr);
    ASSERT_EQ(req.input->GetInputType(), InputType::kItems);
    auto items = std::static_pointer_cast<InputItems>(req.input);
    ASSERT_EQ(items->items.size(), 1);
    EXPECT_TRUE(items->items[0]->ToString().find("func") != std::string::npos);
}

TEST(JsonTest, DeserializeChatRequest_ToolResultInArray) {
    json j = {{"model", "gpt-4o"},
              {"input", json::array({{{"callId", "call_1"}, {"name", "func"}, {"output", "result_string"}}})}};

    ChatRequest req = j.get<ChatRequest>();

    ASSERT_NE(req.input, nullptr);
    ASSERT_EQ(req.input->GetInputType(), InputType::kItems);
    auto items = std::static_pointer_cast<InputItems>(req.input);
    ASSERT_EQ(items->items.size(), 1);
    EXPECT_TRUE(items->items[0]->ToString().find("func") != std::string::npos);
}

TEST(JsonTest, DeserializeChatRequest_ArgumentsAsObject) {
    // When arguments is a JSON object (not string), it should be dumped to string
    json j = {{"model", "gpt-4o"},
              {"input", json::array({{{"callId", "c1"}, {"name", "fn"}, {"arguments", {{"x", 42}}}}})}};

    ChatRequest req = j.get<ChatRequest>();

    ASSERT_NE(req.input, nullptr);
    auto items = std::static_pointer_cast<InputItems>(req.input);
    // The ToolCallRequest stores arguments as a string (dumped from object)
    EXPECT_TRUE(items->items[0]->ToString().find("fn") != std::string::npos);
}

TEST(JsonTest, DeserializeChatRequest_WebSearchTool) {
    json j = {{"model", "gpt-4o"}, {"tools", json::array({{{"type", "web_search"}}})}};

    ChatRequest req = j.get<ChatRequest>();

    ASSERT_EQ(req.tools.size(), 1);
    EXPECT_EQ(req.tools[0]->GetToolType(), ToolType::kWebSearch);
    EXPECT_EQ(req.tools[0]->ToolName(), "web_search");
}

TEST(JsonTest, DeserializeChatRequest_ToolWithoutParameters) {
    json j = {{"model", "gpt-4o"},
              {"tools", json::array({{{"type", "function"}, {"name", "my_func"}, {"description", "desc"}}})}};

    ChatRequest req = j.get<ChatRequest>();

    ASSERT_EQ(req.tools.size(), 1);
    EXPECT_EQ(req.tools[0]->ToolName(), "my_func");
    // Without "parameters" key, should fallback to empty object
    auto ft = std::static_pointer_cast<FunctionTool>(req.tools[0]);
    EXPECT_TRUE(ft->parameters.is_object());
    EXPECT_TRUE(ft->parameters.empty());
}

TEST(JsonTest, DeserializeChatRequest_FileAndImageArrays) {
    json j = {{"model", "gpt-4o"},
              {"fileIDs", json::array({"f1", "f2"})},
              {"imageIDs", json::array({"i1"})},
              {"imageURLs", json::array({"https://example.com/img.png"})}};

    ChatRequest req = j.get<ChatRequest>();

    ASSERT_EQ(req.file_ids.size(), 2);
    EXPECT_EQ(req.file_ids[0], "f1");
    EXPECT_EQ(req.file_ids[1], "f2");
    ASSERT_EQ(req.image_ids.size(), 1);
    EXPECT_EQ(req.image_ids[0], "i1");
    ASSERT_EQ(req.image_urls.size(), 1);
    EXPECT_EQ(req.image_urls[0], "https://example.com/img.png");
}

TEST(JsonTest, DeserializeChatResponse_ArgumentsAsObject) {
    json j = {{"text", ""}, {"toolCallRequests", {{{"callId", "c1"}, {"name", "fn"}, {"arguments", {{"x", 1}}}}}}};

    ChatResponse resp = j.get<ChatResponse>();

    ASSERT_EQ(resp.tool_call_requests.size(), 1);
    EXPECT_EQ(resp.tool_call_requests[0].arguments, R"({"x":1})");
}

TEST(JsonTest, DeserializeChatResponse_InternalToolCalls) {
    json j = {{"text", "Based on my research..."},
              {"responseID", "resp_1"},
              {"tokensUsed", 200},
              {"internalToolCalls",
               json::array({{{"type", "web_search"}, {"query", "ESP32 power consumption deep sleep"}}})}};

    ChatResponse resp = j.get<ChatResponse>();

    EXPECT_EQ(resp.text, "Based on my research...");
    ASSERT_EQ(resp.tools_called.size(), 1);
    EXPECT_EQ(resp.tools_called[0]->ToolName(), "web_search");
    auto ws = std::static_pointer_cast<WebSearchToolCall>(resp.tools_called[0]);
    EXPECT_EQ(ws->query, "ESP32 power consumption deep sleep");
}

TEST(JsonTest, DeserializeChatResponse_InternalToolCalls_UnknownTypeIgnored) {
    json j = {{"text", "Hello"},
              {"internalToolCalls", json::array({{{"type", "code_interpreter"}, {"code", "print(1)"}}})}};

    ChatResponse resp = j.get<ChatResponse>();

    EXPECT_EQ(resp.text, "Hello");
    EXPECT_TRUE(resp.tools_called.empty());
}

TEST(JsonTest, SerializeOptionsAllFields) {
    Options opts;
    opts.WithMaxTokens(100)
        .WithTemperature(0.7f)
        .WithTopK(40)
        .WithTopP(0.9f)
        .WithFrequencyPenalty(0.5f)
        .WithPresencePenalty(0.3f)
        .WithSeed(42);

    json j;
    foresthub::core::to_json(j, opts);

    EXPECT_EQ(j["maxTokens"], 100);
    EXPECT_FLOAT_EQ(j["temperature"].get<float>(), 0.7f);
    EXPECT_EQ(j["topK"], 40);
    EXPECT_FLOAT_EQ(j["topP"].get<float>(), 0.9f);
    EXPECT_FLOAT_EQ(j["frequencyPenalty"].get<float>(), 0.5f);
    EXPECT_FLOAT_EQ(j["presencePenalty"].get<float>(), 0.3f);
    EXPECT_EQ(j["seed"], 42);

    // Roundtrip deserialization
    Options opts2;
    foresthub::core::from_json(j, opts2);

    ASSERT_TRUE(opts2.max_tokens.HasValue());
    EXPECT_EQ(*opts2.max_tokens, 100);
    ASSERT_TRUE(opts2.top_k.HasValue());
    EXPECT_EQ(*opts2.top_k, 40);
    ASSERT_TRUE(opts2.frequency_penalty.HasValue());
    EXPECT_FLOAT_EQ(*opts2.frequency_penalty, 0.5f);
    ASSERT_TRUE(opts2.presence_penalty.HasValue());
    EXPECT_FLOAT_EQ(*opts2.presence_penalty, 0.3f);
    ASSERT_TRUE(opts2.seed.HasValue());
    EXPECT_EQ(*opts2.seed, 42);
}

TEST(JsonTest, DeserializeChatRequest_WithResponseFormat) {
    json j = {{"model", "gpt-4o"},
              {"input", {{"value", "Hi"}}},
              {"responseFormat", {{"name", "weather"}, {"schema", {{"type", "object"}}}, {"description", "desc"}}}};

    ChatRequest req = j.get<ChatRequest>();

    ASSERT_TRUE(req.response_format.HasValue());
    EXPECT_EQ(req.response_format->name, "weather");
    EXPECT_EQ(req.response_format->description, "desc");
    EXPECT_EQ(req.response_format->schema["type"], "object");
}

TEST(JsonTest, DeserializeChatRequest_ToolWithParameters) {
    json j = {{"model", "gpt-4o"},
              {"tools",
               json::array({{{"type", "function"},
                             {"name", "calc"},
                             {"description", "math"},
                             {"parameters", {{"type", "object"}, {"properties", {{"x", {{"type", "number"}}}}}}}}})}};

    ChatRequest req = j.get<ChatRequest>();

    ASSERT_EQ(req.tools.size(), 1);
    EXPECT_EQ(req.tools[0]->ToolName(), "calc");
    auto ft = std::static_pointer_cast<FunctionTool>(req.tools[0]);
    EXPECT_FALSE(ft->parameters.empty());
    EXPECT_EQ(ft->parameters["type"], "object");
}
