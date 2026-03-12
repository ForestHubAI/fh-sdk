// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_TESTS_MOCK_PROVIDER_HPP
#define FORESTHUB_TESTS_MOCK_PROVIDER_HPP

#include <algorithm>
#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "foresthub/core/provider.hpp"

namespace foresthub {
namespace tests {

class MockProvider : public core::Provider {
public:
    std::string provider_id = "mock-provider";
    std::vector<std::string> models;
    std::string health_result;
    std::deque<std::shared_ptr<core::ChatResponse>> responses;
    int call_count = 0;
    core::ChatRequest last_request;

    core::ProviderID ProviderId() const override { return provider_id; }

    std::string Health() const override { return health_result; }

    bool SupportsModel(const core::ModelID& model) const override {
        return std::find(models.begin(), models.end(), model) != models.end();
    }

    std::shared_ptr<core::ChatResponse> Chat(const core::ChatRequest& req) override {
        ++call_count;
        last_request = req;
        if (responses.empty()) {
            return nullptr;
        }
        std::shared_ptr<core::ChatResponse> resp = responses.front();
        responses.pop_front();
        return resp;
    }
};

}  // namespace tests
}  // namespace foresthub

#endif  // FORESTHUB_TESTS_MOCK_PROVIDER_HPP
