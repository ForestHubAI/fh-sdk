// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_TESTS_MOCK_LLM_CLIENT_HPP
#define FORESTHUB_TESTS_MOCK_LLM_CLIENT_HPP

#include <deque>
#include <memory>
#include <vector>

#include "foresthub/llm/provider.hpp"

namespace foresthub {
namespace tests {

class MockChatClient : public llm::ChatClient {
public:
    std::deque<std::shared_ptr<llm::ChatResponse>> responses;
    int call_count = 0;
    std::vector<llm::ChatRequest> captured_requests;

    std::shared_ptr<llm::ChatResponse> Chat(const llm::ChatRequest& req) override {
        ++call_count;
        captured_requests.push_back(req);
        if (responses.empty()) {
            return nullptr;
        }
        std::shared_ptr<llm::ChatResponse> resp = responses.front();
        responses.pop_front();
        return resp;
    }
};

}  // namespace tests
}  // namespace foresthub

#endif  // FORESTHUB_TESTS_MOCK_LLM_CLIENT_HPP
