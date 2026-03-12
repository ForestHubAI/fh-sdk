// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "foresthub/provider/remote/foresthub.hpp"

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

// Helper: create a ForestHubProvider with the given MockHttpClient and config.
struct TestFixture {
    std::shared_ptr<tests::MockHttpClient> mock_http = std::make_shared<tests::MockHttpClient>();
    config::ProviderConfig cfg;

    TestFixture() {
        cfg.base_url = "https://api.example.com";
        cfg.api_key = "test-key-123";
        cfg.supported_models = {"gpt-4", "gpt-4o"};
    }

    ForestHubProvider MakeProvider() { return ForestHubProvider(cfg, mock_http); }

    // Convenience: a valid JSON chat response body.
    static std::string ValidResponseBody() {
        json j = {{"text", "Hello world"}, {"responseID", "resp-1"}, {"tokensUsed", 42}};
        return j.dump();
    }

    // A response body with tool calls.
    static std::string ToolCallResponseBody() {
        json j = {{"text", ""},
                  {"responseID", "resp-2"},
                  {"tokensUsed", 10},
                  {"toolCallRequests",
                   {{{"callId", "call-1"}, {"name", "get_weather"}, {"arguments", {{"city", "Berlin"}}}}}}};
        return j.dump();
    }
};

// --- Construction & Identity ---

TEST(ForestHubProvider, Construction) {
    TestFixture fixture;
    ForestHubProvider provider = fixture.MakeProvider();

    EXPECT_EQ(provider.ProviderId(), "forest-hub");
}

TEST(ForestHubProvider, BaseUrlTrailingSlashStripped) {
    TestFixture fixture;
    fixture.cfg.base_url = "https://api.example.com/";
    fixture.mock_http->get_responses.push_back({200, "OK", {}});
    ForestHubProvider provider = fixture.MakeProvider();

    // Verify via Health URL (no double slash)
    provider.Health();
    EXPECT_EQ(fixture.mock_http->last_url, "https://api.example.com/llm/health");
}

// --- SupportsModel ---

TEST(ForestHubProvider, SupportsModelFound) {
    TestFixture fixture;
    ForestHubProvider provider = fixture.MakeProvider();

    EXPECT_TRUE(provider.SupportsModel("gpt-4"));
    EXPECT_TRUE(provider.SupportsModel("gpt-4o"));
}

TEST(ForestHubProvider, SupportsModelNotFound) {
    TestFixture fixture;
    ForestHubProvider provider = fixture.MakeProvider();

    EXPECT_FALSE(provider.SupportsModel("unknown-model"));
}

TEST(ForestHubProvider, SupportsModelEmptyList) {
    TestFixture fixture;
    fixture.cfg.supported_models.clear();
    ForestHubProvider provider = fixture.MakeProvider();

    EXPECT_FALSE(provider.SupportsModel("gpt-4"));
}

// --- Health ---

TEST(ForestHubProvider, HealthSuccess) {
    TestFixture fixture;
    fixture.mock_http->get_responses.push_back({200, "OK", {}});
    ForestHubProvider provider = fixture.MakeProvider();

    std::string err = provider.Health();
    EXPECT_TRUE(err.empty());
}

TEST(ForestHubProvider, HealthFailure) {
    TestFixture fixture;
    fixture.mock_http->get_responses.push_back({500, "Internal Server Error", {}});
    ForestHubProvider provider = fixture.MakeProvider();

    std::string err = provider.Health();
    EXPECT_FALSE(err.empty());
    EXPECT_NE(err.find("500"), std::string::npos);
}

TEST(ForestHubProvider, HealthUrlCorrect) {
    TestFixture fixture;
    fixture.mock_http->get_responses.push_back({200, "OK", {}});
    ForestHubProvider provider = fixture.MakeProvider();

    provider.Health();

    EXPECT_EQ(fixture.mock_http->last_url, "https://api.example.com/llm/health");
}

// --- Chat: Happy Path ---

TEST(ForestHubProvider, ChatSuccess) {
    TestFixture fixture;
    fixture.mock_http->post_responses.push_back({200, TestFixture::ValidResponseBody(), {}});
    ForestHubProvider provider = fixture.MakeProvider();

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("gpt-4", input);
    std::shared_ptr<core::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->text, "Hello world");
    EXPECT_EQ(resp->response_id, "resp-1");
    EXPECT_EQ(resp->tokens_used, 42);
}

TEST(ForestHubProvider, ChatUrlCorrect) {
    TestFixture fixture;
    fixture.mock_http->post_responses.push_back({200, TestFixture::ValidResponseBody(), {}});
    ForestHubProvider provider = fixture.MakeProvider();

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("gpt-4", input);
    provider.Chat(req);

    EXPECT_EQ(fixture.mock_http->last_url, "https://api.example.com/llm/generate");
}

TEST(ForestHubProvider, ChatHeadersCorrect) {
    TestFixture fixture;
    fixture.mock_http->post_responses.push_back({200, TestFixture::ValidResponseBody(), {}});
    ForestHubProvider provider = fixture.MakeProvider();

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("gpt-4", input);
    provider.Chat(req);

    EXPECT_EQ(fixture.mock_http->last_headers["Content-Type"], "application/json");
    EXPECT_EQ(fixture.mock_http->last_headers["Device-Key"], "test-key-123");
    EXPECT_EQ(fixture.mock_http->last_headers["Accept"], "application/json");
}

TEST(ForestHubProvider, ChatRequestBodyIsJson) {
    TestFixture fixture;
    fixture.mock_http->post_responses.push_back({200, TestFixture::ValidResponseBody(), {}});
    ForestHubProvider provider = fixture.MakeProvider();

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("gpt-4", input);
    provider.Chat(req);

    json body = json::parse(fixture.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    EXPECT_EQ(body.value("model", ""), "gpt-4");
}

// --- Chat: Tool Call Response ---

TEST(ForestHubProvider, ChatWithToolCalls) {
    TestFixture fixture;
    fixture.mock_http->post_responses.push_back({200, TestFixture::ToolCallResponseBody(), {}});
    ForestHubProvider provider = fixture.MakeProvider();

    auto input = std::make_shared<core::InputString>("What is the weather?");
    core::ChatRequest req("gpt-4", input);
    std::shared_ptr<core::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    ASSERT_EQ(resp->tool_call_requests.size(), 1u);
    EXPECT_EQ(resp->tool_call_requests[0].call_id, "call-1");
    EXPECT_EQ(resp->tool_call_requests[0].name, "get_weather");
}

// --- Chat: Error Paths ---

TEST(ForestHubProvider, ChatServerError5xx) {
    TestFixture fixture;
    // Both attempts return 500.
    fixture.mock_http->post_responses.push_back({500, "error", {}});
    fixture.mock_http->post_responses.push_back({500, "error", {}});
    ForestHubProvider provider = fixture.MakeProvider();

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("gpt-4", input);
    std::shared_ptr<core::ChatResponse> resp = provider.Chat(req);

    EXPECT_EQ(resp, nullptr);
}

TEST(ForestHubProvider, ChatClientError4xx) {
    TestFixture fixture;
    // 400 — no retry.
    fixture.mock_http->post_responses.push_back({400, "bad request", {}});
    ForestHubProvider provider = fixture.MakeProvider();

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("gpt-4", input);
    std::shared_ptr<core::ChatResponse> resp = provider.Chat(req);

    EXPECT_EQ(resp, nullptr);
}

TEST(ForestHubProvider, ChatRateLimited429) {
    TestFixture fixture;
    // First: 429 (retryable), second: 200.
    fixture.mock_http->post_responses.push_back({429, "rate limited", {}});
    fixture.mock_http->post_responses.push_back({200, TestFixture::ValidResponseBody(), {}});
    ForestHubProvider provider = fixture.MakeProvider();

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("gpt-4", input);
    std::shared_ptr<core::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->text, "Hello world");
}

TEST(ForestHubProvider, ChatTimeout408) {
    TestFixture fixture;
    // First: 408 (retryable), second: 200.
    fixture.mock_http->post_responses.push_back({408, "timeout", {}});
    fixture.mock_http->post_responses.push_back({200, TestFixture::ValidResponseBody(), {}});
    ForestHubProvider provider = fixture.MakeProvider();

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("gpt-4", input);
    std::shared_ptr<core::ChatResponse> resp = provider.Chat(req);

    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->text, "Hello world");
}

TEST(ForestHubProvider, ChatMalformedJson) {
    TestFixture fixture;
    fixture.mock_http->post_responses.push_back({200, "not valid json {{{", {}});
    ForestHubProvider provider = fixture.MakeProvider();

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("gpt-4", input);
    std::shared_ptr<core::ChatResponse> resp = provider.Chat(req);

    EXPECT_EQ(resp, nullptr);
}

TEST(ForestHubProvider, ChatEmptyBody) {
    TestFixture fixture;
    fixture.mock_http->post_responses.push_back({200, "", {}});
    ForestHubProvider provider = fixture.MakeProvider();

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("gpt-4", input);
    std::shared_ptr<core::ChatResponse> resp = provider.Chat(req);

    EXPECT_EQ(resp, nullptr);
}

// --- Retry Logic ---

TEST(ForestHubProvider, ChatMaxRetries) {
    TestFixture fixture;
    // Both attempts return 500.
    fixture.mock_http->post_responses.push_back({500, "error", {}});
    fixture.mock_http->post_responses.push_back({500, "error", {}});
    ForestHubProvider provider = fixture.MakeProvider();

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("gpt-4", input);
    provider.Chat(req);

    EXPECT_EQ(fixture.mock_http->post_call_count, 2);
}

TEST(ForestHubProvider, ChatNoRetryOn4xx) {
    TestFixture fixture;
    fixture.mock_http->post_responses.push_back({403, "forbidden", {}});
    ForestHubProvider provider = fixture.MakeProvider();

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("gpt-4", input);
    provider.Chat(req);

    EXPECT_EQ(fixture.mock_http->post_call_count, 1);
}

TEST(ForestHubProvider, ChatDelayCalledOnRetry) {
    struct DelayTrackingHttpClient : public tests::MockHttpClient {
        int delay_call_count = 0;
        void Delay(unsigned long /*ms*/) override { ++delay_call_count; }
    };

    auto mock = std::make_shared<DelayTrackingHttpClient>();
    mock->post_responses.push_back({500, "error", {}});
    mock->post_responses.push_back({200, TestFixture::ValidResponseBody(), {}});

    config::ProviderConfig cfg;
    cfg.base_url = "https://api.example.com";
    cfg.api_key = "key";
    cfg.supported_models = {"gpt-4"};

    ForestHubProvider provider(cfg, mock);

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("gpt-4", input);
    provider.Chat(req);

    EXPECT_GE(mock->delay_call_count, 1);
}

}  // namespace
}  // namespace remote
}  // namespace provider
}  // namespace foresthub
