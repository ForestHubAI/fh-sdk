#include "foresthub/client.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "mocks/mock_http_client.hpp"
#include "mocks/mock_provider.hpp"

using namespace foresthub;
using namespace foresthub::core;

// ==========================================================================
// 1. Construction and Registration
// ==========================================================================

TEST(ClientTest, DefaultConstruction) {
    Client client;
    EXPECT_TRUE(client.providers().empty());
}

TEST(ClientTest, RegisterProvider) {
    Client client;
    auto mock = std::make_shared<tests::MockProvider>();
    mock->provider_id = "test-provider";
    client.RegisterProvider(mock);
    EXPECT_EQ(client.providers().size(), 1u);
    EXPECT_NE(client.providers().find("test-provider"), client.providers().end());
}

TEST(ClientTest, RegisterNullProvider) {
    Client client;
    client.RegisterProvider(nullptr);
    EXPECT_TRUE(client.providers().empty());
}

// ==========================================================================
// 2. SupportsModel
// ==========================================================================

TEST(ClientTest, SupportsModel_Found) {
    Client client;
    auto mock = std::make_shared<tests::MockProvider>();
    mock->models = {"gpt-4o"};
    client.RegisterProvider(mock);
    EXPECT_TRUE(client.SupportsModel("gpt-4o"));
}

TEST(ClientTest, SupportsModel_NotFound) {
    Client client;
    auto mock = std::make_shared<tests::MockProvider>();
    mock->models = {"gpt-4o"};
    client.RegisterProvider(mock);
    EXPECT_FALSE(client.SupportsModel("unknown-model"));
}

// ==========================================================================
// 3. Chat Routing
// ==========================================================================

TEST(ClientTest, Chat_RoutesToCorrectProvider) {
    Client client;

    auto mock_a = std::make_shared<tests::MockProvider>();
    mock_a->provider_id = "provider-a";
    mock_a->models = {"model-a"};
    auto response_a = std::make_shared<ChatResponse>();
    response_a->text = "from A";
    mock_a->responses.push_back(response_a);

    auto mock_b = std::make_shared<tests::MockProvider>();
    mock_b->provider_id = "provider-b";
    mock_b->models = {"model-b"};
    auto response_b = std::make_shared<ChatResponse>();
    response_b->text = "from B";
    mock_b->responses.push_back(response_b);

    client.RegisterProvider(mock_a);
    client.RegisterProvider(mock_b);

    auto input = std::make_shared<InputString>("hello");
    ChatRequest req("model-b", input);
    std::shared_ptr<ChatResponse> result = client.Chat(req);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->text, "from B");
    EXPECT_EQ(mock_b->call_count, 1);
    EXPECT_EQ(mock_a->call_count, 0);
}

TEST(ClientTest, Chat_UnknownModel) {
    Client client;
    auto mock = std::make_shared<tests::MockProvider>();
    mock->models = {"gpt-4o"};
    client.RegisterProvider(mock);

    auto input = std::make_shared<InputString>("hello");
    ChatRequest req("nonexistent-model", input);
    std::shared_ptr<ChatResponse> result = client.Chat(req);

    EXPECT_EQ(result, nullptr);
    EXPECT_EQ(mock->call_count, 0);
}

// ==========================================================================
// 4. Health
// ==========================================================================

TEST(ClientTest, Health_NoProviders) {
    Client client;
    EXPECT_EQ(client.Health(), "No providers configured");
}

TEST(ClientTest, Health_AllHealthy) {
    Client client;
    auto mock = std::make_shared<tests::MockProvider>();
    mock->health_result = "";
    client.RegisterProvider(mock);
    EXPECT_TRUE(client.Health().empty());
}

TEST(ClientTest, Health_OneUnhealthy) {
    Client client;
    auto mock = std::make_shared<tests::MockProvider>();
    mock->health_result = "connection timeout";
    client.RegisterProvider(mock);
    std::string health = client.Health();
    EXPECT_FALSE(health.empty());
    EXPECT_TRUE(health.find("connection timeout") != std::string::npos);
}

TEST(ClientTest, Health_MultipleProviders) {
    Client client;

    auto healthy = std::make_shared<tests::MockProvider>();
    healthy->provider_id = "healthy";
    healthy->health_result = "";
    client.RegisterProvider(healthy);

    auto unhealthy = std::make_shared<tests::MockProvider>();
    unhealthy->provider_id = "unhealthy";
    unhealthy->health_result = "down";
    client.RegisterProvider(unhealthy);

    std::string health = client.Health();
    EXPECT_TRUE(health.find("unhealthy") != std::string::npos);
    EXPECT_TRUE(health.find("down") != std::string::npos);
    // The healthy provider should not appear in the error string.
    EXPECT_TRUE(health.find("healthy:") == std::string::npos || health.find("unhealthy") != std::string::npos);
}

// ==========================================================================
// 5. Factory
// ==========================================================================

TEST(ClientTest, CreateFactory_WithRemoteConfig) {
    config::ClientConfig cfg;
    config::ProviderConfig fh;
    fh.base_url = "https://example.com";
    fh.api_key = "test-key";
    fh.supported_models = {"gpt-4o"};
    cfg.remote.foresthub = fh;

    auto http = std::make_shared<tests::MockHttpClient>();
    std::unique_ptr<Client> client = Client::Create(cfg, http);

    ASSERT_NE(client, nullptr);
    EXPECT_FALSE(client->providers().empty());
}

TEST(ClientTest, CreateFactory_EmptyConfig) {
    config::ClientConfig cfg;
    std::unique_ptr<Client> client = Client::Create(cfg);
    ASSERT_NE(client, nullptr);
    EXPECT_TRUE(client->providers().empty());
}

TEST(ClientTest, Health_MultipleUnhealthy) {
    Client client;

    auto unhealthy_a = std::make_shared<tests::MockProvider>();
    unhealthy_a->provider_id = "provider-a";
    unhealthy_a->health_result = "timeout";
    client.RegisterProvider(unhealthy_a);

    auto unhealthy_b = std::make_shared<tests::MockProvider>();
    unhealthy_b->provider_id = "provider-b";
    unhealthy_b->health_result = "refused";
    client.RegisterProvider(unhealthy_b);

    std::string health = client.Health();
    // Multiple failures are joined with "; " separator.
    EXPECT_TRUE(health.find("; ") != std::string::npos);
    EXPECT_TRUE(health.find("timeout") != std::string::npos);
    EXPECT_TRUE(health.find("refused") != std::string::npos);
}
