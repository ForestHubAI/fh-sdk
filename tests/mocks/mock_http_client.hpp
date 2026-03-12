// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_TESTS_MOCK_HTTP_CLIENT_HPP
#define FORESTHUB_TESTS_MOCK_HTTP_CLIENT_HPP

#include <deque>
#include <string>

#include "foresthub/core/http_client.hpp"

namespace foresthub {
namespace tests {

class MockHttpClient : public core::HttpClient {
public:
    std::deque<core::HttpResponse> get_responses;
    std::deque<core::HttpResponse> post_responses;
    int get_call_count = 0;
    int post_call_count = 0;
    std::string last_url;
    std::string last_body;
    Headers last_headers;

    core::HttpResponse Get(const std::string& url, const Headers& headers) override {
        ++get_call_count;
        last_url = url;
        last_headers = headers;
        if (get_responses.empty()) {
            return {500, "no mock response", {}};
        }
        core::HttpResponse resp = get_responses.front();
        get_responses.pop_front();
        return resp;
    }

    core::HttpResponse Post(const std::string& url, const Headers& headers, const std::string& body) override {
        ++post_call_count;
        last_url = url;
        last_headers = headers;
        last_body = body;
        if (post_responses.empty()) {
            return {500, "no mock response", {}};
        }
        core::HttpResponse resp = post_responses.front();
        post_responses.pop_front();
        return resp;
    }

    void Delay(unsigned long /*ms*/) override {}
};

}  // namespace tests
}  // namespace foresthub

#endif  // FORESTHUB_TESTS_MOCK_HTTP_CLIENT_HPP
