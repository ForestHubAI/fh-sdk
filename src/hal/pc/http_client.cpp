// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "http_client.hpp"

#include <cpr/cpr.h>

#include <chrono>
#include <thread>

namespace foresthub {
namespace hal {
namespace pc {

PcHttpClient::PcHttpClient(int timeout_ms) : timeout_ms_(timeout_ms) {}

// --- Helper Functions ---

// Converts the library-agnostic 'HttpClient::Headers' (std::map)
// into the specific 'cpr::Header' format required by the CPR library.
static cpr::Header ToCprHeader(const HttpClient::Headers& headers) {
    cpr::Header cpr_headers;
    for (const auto& entry : headers) {
        cpr_headers.insert({entry.first, entry.second});
    }
    return cpr_headers;
}

// Converts the specific CPR response back into the generic 'HttpResponse'.
static HttpResponse FromCprResponse(const cpr::Response& cpr_response) {
    HttpResponse resp;

    // Explicitly cast to int to match the domain struct
    resp.status_code = static_cast<int>(cpr_response.status_code);
    resp.body = cpr_response.text;

    // Copy headers back to the generic std::map
    for (const auto& entry : cpr_response.header) {
        resp.headers[entry.first] = entry.second;
    }

    // Handle network-level errors (e.g., DNS failure, Connection Refused).
    // libcurl/CPR returns 0 if no HTTP response was received.
    // Map this to HTTP 503 (Service Unavailable) so the application logic
    // can handle it as a standard failure.
    if (cpr_response.status_code == 0) {
        resp.status_code = 503;
        resp.body = "Network Error: " + cpr_response.error.message;
    }

    return resp;
}

// --- Interface Implementation ---

HttpResponse PcHttpClient::Get(const std::string& url, const Headers& headers) {
    cpr::Response response = cpr::Get(cpr::Url{url}, ToCprHeader(headers), cpr::Timeout{timeout_ms_});
    return FromCprResponse(response);
}

HttpResponse PcHttpClient::Post(const std::string& url, const Headers& headers, const std::string& body) {
    cpr::Response response = cpr::Post(cpr::Url{url}, ToCprHeader(headers), cpr::Body{body}, cpr::Timeout{timeout_ms_});
    return FromCprResponse(response);
}

void PcHttpClient::Delay(unsigned long ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

}  // namespace pc
}  // namespace hal
}  // namespace foresthub
