// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_HAL_HTTP_CLIENT_HPP
#define FORESTHUB_HAL_HTTP_CLIENT_HPP

/// @file
/// Abstract HTTP client interface and response type.

#include <cstdint>
#include <map>
#include <string>

namespace foresthub {
namespace hal {

/// Call-time configuration for HTTP client creation.
///
/// Passed to hal::CreateHttpClient() to specify connection parameters.
/// Host is required; all other fields have sensible defaults.
struct HttpClientConfig {
    const char* host = nullptr;        ///< Target hostname (required).
    uint16_t port = 443;               ///< Target port number.
    bool use_tls = true;               ///< Whether to use TLS encryption.
    unsigned long timeout_ms = 30000;  ///< Request timeout in milliseconds.
};

/// Simple HTTP response container.
struct HttpResponse {
    int status_code = 0;                         ///< HTTP status code (e.g., 200, 404, 500).
    std::string body;                            ///< Response body (usually JSON).
    std::map<std::string, std::string> headers;  ///< Response headers.
};

/// Abstract interface for HTTP operations.
class HttpClient {
public:
    virtual ~HttpClient() = default;

    using Headers = std::map<std::string, std::string>;

    /// Perform a GET request.
    /// @param url Full URL to request.
    /// @param headers HTTP headers to send.
    /// @return HTTP response; status_code is 503 with a descriptive body on network-level failure.
    virtual HttpResponse Get(const std::string& url, const Headers& headers) = 0;

    /// Perform a POST request with a raw body.
    /// @param url Full URL to request.
    /// @param headers HTTP headers to send.
    /// @param body Raw request body (typically JSON string).
    /// @return HTTP response; status_code is 503 with a descriptive body on network-level failure.
    virtual HttpResponse Post(const std::string& url, const Headers& headers, const std::string& body) = 0;

    /// Delay execution for retry logic.
    /// @param ms Delay duration in milliseconds.
    virtual void Delay(unsigned long ms) = 0;
};

}  // namespace hal
}  // namespace foresthub

#endif  // FORESTHUB_HAL_HTTP_CLIENT_HPP
