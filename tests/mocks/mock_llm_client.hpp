#ifndef FORESTHUB_TESTS_MOCK_LLM_CLIENT_HPP
#define FORESTHUB_TESTS_MOCK_LLM_CLIENT_HPP

#include <deque>
#include <memory>
#include <vector>

#include "foresthub/core/provider.hpp"

namespace foresthub {
namespace tests {

class MockLLMClient : public core::LLMClient {
public:
    std::deque<std::shared_ptr<core::ChatResponse>> responses;
    int call_count = 0;
    std::vector<core::ChatRequest> captured_requests;

    std::shared_ptr<core::ChatResponse> Chat(const core::ChatRequest& req) override {
        ++call_count;
        captured_requests.push_back(req);
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

#endif  // FORESTHUB_TESTS_MOCK_LLM_CLIENT_HPP
