#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "foresthub/client.hpp"
#include "foresthub/core/input.hpp"
#include "foresthub/core/types.hpp"
#include "mocks/mock_provider.hpp"

namespace foresthub {
namespace {

// --- Client multi-provider routing ---

TEST(ClientRoutingIntegrationTest, RoutesToCorrectProvider) {
    auto provider_a = std::make_shared<tests::MockProvider>();
    provider_a->provider_id = "provider-a";
    provider_a->models = {"model-a"};
    auto resp_a = std::make_shared<core::ChatResponse>();
    resp_a->text = "from A";
    provider_a->responses.push_back(resp_a);

    auto provider_b = std::make_shared<tests::MockProvider>();
    provider_b->provider_id = "provider-b";
    provider_b->models = {"model-b"};
    auto resp_b = std::make_shared<core::ChatResponse>();
    resp_b->text = "from B";
    provider_b->responses.push_back(resp_b);

    auto client = std::make_unique<Client>();
    client->RegisterProvider(provider_a);
    client->RegisterProvider(provider_b);

    // Route to A.
    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req_a("model-a", input);
    std::shared_ptr<core::ChatResponse> result_a = client->Chat(req_a);

    ASSERT_NE(result_a, nullptr);
    EXPECT_EQ(result_a->text, "from A");
    EXPECT_EQ(provider_a->call_count, 1);
    EXPECT_EQ(provider_b->call_count, 0);

    // Route to B.
    core::ChatRequest req_b("model-b", input);
    std::shared_ptr<core::ChatResponse> result_b = client->Chat(req_b);

    ASSERT_NE(result_b, nullptr);
    EXPECT_EQ(result_b->text, "from B");
    EXPECT_EQ(provider_b->call_count, 1);
}

TEST(ClientRoutingIntegrationTest, FirstMatchWins) {
    // Two providers both support the same model.
    auto provider_first = std::make_shared<tests::MockProvider>();
    provider_first->provider_id = "first";
    provider_first->models = {"shared-model"};
    auto resp_first = std::make_shared<core::ChatResponse>();
    resp_first->text = "first wins";
    provider_first->responses.push_back(resp_first);

    auto provider_second = std::make_shared<tests::MockProvider>();
    provider_second->provider_id = "second";
    provider_second->models = {"shared-model"};

    auto client = std::make_unique<Client>();
    client->RegisterProvider(provider_first);
    client->RegisterProvider(provider_second);

    auto input = std::make_shared<core::InputString>("Hello");
    core::ChatRequest req("shared-model", input);
    std::shared_ptr<core::ChatResponse> result = client->Chat(req);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->text, "first wins");
    EXPECT_EQ(provider_first->call_count, 1);
    EXPECT_EQ(provider_second->call_count, 0);
}

TEST(ClientRoutingIntegrationTest, HealthAggregation) {
    auto healthy = std::make_shared<tests::MockProvider>();
    healthy->provider_id = "healthy";
    healthy->models = {"m1"};
    healthy->health_result = "";  // Healthy.

    auto unhealthy = std::make_shared<tests::MockProvider>();
    unhealthy->provider_id = "unhealthy";
    unhealthy->models = {"m2"};
    unhealthy->health_result = "connection refused";

    auto client = std::make_unique<Client>();
    client->RegisterProvider(healthy);
    client->RegisterProvider(unhealthy);

    std::string health = client->Health();

    // Should contain the unhealthy provider's error.
    EXPECT_NE(health.find("unhealthy"), std::string::npos);
    EXPECT_NE(health.find("connection refused"), std::string::npos);
}

}  // namespace
}  // namespace foresthub
