// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#ifndef FORESTHUB_PLATFORM_ARDUINO_HTTP_CLIENT_HPP
#define FORESTHUB_PLATFORM_ARDUINO_HTTP_CLIENT_HPP

#ifdef FORESTHUB_ENABLE_NETWORK

#include <memory>
#include <string>

#include "board_wifi.hpp"
#include "foresthub/hal/crypto.hpp"
#include "foresthub/hal/http_client.hpp"

namespace foresthub {
namespace hal {
namespace arduino {

/// HTTP client implementation using ArduinoHttpClient with yield()-based WDT safety.
class ArduinoHttpClient : public HttpClient {
public:
    /// Construct HTTP client for TLS connections.
    ArduinoHttpClient(std::shared_ptr<TLSClientWrapper> tls_wrapper, const char* host, uint16_t port,
                      unsigned long timeout_ms = 60000);

    /// Construct HTTP client for non-TLS connections.
    ArduinoHttpClient(std::unique_ptr<Client> plain_client, const char* host, uint16_t port,
                      unsigned long timeout_ms = 60000);

    ~ArduinoHttpClient() override;

    /// Sends GET via ArduinoHttpClient with yield() in wait loops.
    HttpResponse Get(const std::string& url, const Headers& headers) override;
    /// Sends POST via ArduinoHttpClient with yield() in wait loops.
    HttpResponse Post(const std::string& url, const Headers& headers, const std::string& body) override;
    /// Calls yield() before sleeping to prevent WDT resets.
    void Delay(unsigned long ms) override;

private:
    Client* client_;  ///< Active client pointer (never null, points to TLS or plain client)
    std::shared_ptr<TLSClientWrapper> tls_wrapper_;  ///< Owns TLS client (nullptr for non-TLS)
    std::unique_ptr<Client> owned_client_;           ///< Owns plain client (nullptr for TLS)

    std::string host_;          ///< Target hostname passed at construction.
    uint16_t port_;             ///< Target port number.
    unsigned long timeout_ms_;  ///< HTTP request timeout in milliseconds.

    /// Sends request with yield() calls to prevent WDT resets.
    HttpResponse SendRequest(const char* method, const std::string& url, const Headers& headers,
                             const std::string* body = nullptr);
};

}  // namespace arduino
}  // namespace hal
}  // namespace foresthub

#endif  // FORESTHUB_ENABLE_NETWORK

#endif  // FORESTHUB_PLATFORM_ARDUINO_HTTP_CLIENT_HPP
