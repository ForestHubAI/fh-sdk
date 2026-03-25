// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#if defined(FORESTHUB_PLATFORM_ARDUINO) && defined(FORESTHUB_ENABLE_NETWORK)

#include "http_client.hpp"

#include <ArduinoHttpClient.h>

namespace foresthub {
namespace hal {
namespace arduino {

static constexpr int kMaxStallIterations = 200;  // ~1s at 5ms delay per stall cycle

// ----------------------------------------------------------------------------
// Constructors
// ----------------------------------------------------------------------------

ArduinoHttpClient::ArduinoHttpClient(std::shared_ptr<TLSClientWrapper> tls_wrapper, const char* host,
                                                   uint16_t port, unsigned long timeout_ms)
    : client_(static_cast<Client*>(tls_wrapper->GetNativeClient())),
      tls_wrapper_(std::move(tls_wrapper)),
      owned_client_(nullptr),
      host_(host),
      port_(port),
      timeout_ms_(timeout_ms) {}

ArduinoHttpClient::ArduinoHttpClient(std::unique_ptr<Client> plain_client, const char* host,
                                                   uint16_t port, unsigned long timeout_ms)
    : client_(plain_client.get()),
      tls_wrapper_(nullptr),
      owned_client_(std::move(plain_client)),
      host_(host),
      port_(port),
      timeout_ms_(timeout_ms) {}

ArduinoHttpClient::~ArduinoHttpClient() = default;

// ----------------------------------------------------------------------------
// Public Interface
// ----------------------------------------------------------------------------

HttpResponse ArduinoHttpClient::Get(const std::string& url, const Headers& headers) {
    return SendRequest("GET", url, headers);
}

HttpResponse ArduinoHttpClient::Post(const std::string& url, const Headers& headers,
                                          const std::string& body) {
    return SendRequest("POST", url, headers, &body);
}

void ArduinoHttpClient::Delay(unsigned long ms) {
    yield();  // Allow background tasks before blocking
    ::delay(ms);
}

// ----------------------------------------------------------------------------
// Request Implementation
// ----------------------------------------------------------------------------

HttpResponse ArduinoHttpClient::SendRequest(const char* method, const std::string& url,
                                                 const Headers& headers, const std::string* body) {
    // Extract path from URL (ArduinoHttpClient expects path only, not full URL)
    String path = String(url.c_str());
    if (path.startsWith("http")) {
        int path_start = path.indexOf('/', 8);  // Skip 'http://' or 'https://'
        if (path_start > 0) path = path.substring(path_start);
    }

    // Initialize HTTP client
    ::HttpClient http(*client_, host_.c_str(), port_);
    http.setHttpResponseTimeout(timeout_ms_);

    // Connection reset - flush any stale data from previous request
    if (client_->connected()) {
        while (client_->available()) {
            yield();
            client_->read();
        }
        client_->stop();
        yield();
        ::delay(10);
    }

    // Start request
    http.beginRequest();
    if (strcmp(method, "GET") == 0)
        http.get(path);
    else if (strcmp(method, "POST") == 0)
        http.post(path);

    // Send headers
    for (const auto& entry : headers) {
        http.sendHeader(entry.first.c_str(), entry.second.c_str());
    }

    // Add Connection: close if not specified
    if (!headers.count("Connection")) {
        http.sendHeader("Connection", "close");
    }

    // Send body with chunking (prevents buffer overflow on MCUs)
    if (body) {
        http.sendHeader("Content-Length", static_cast<int>(body->length()));
        http.beginBody();

        const auto* data = reinterpret_cast<const uint8_t*>(body->c_str());  // NOLINT -- Arduino API requires uint8_t*
        size_t total = body->length();
        size_t sent = 0;
        int stall_count = 0;

        while (sent < total) {
            yield();  // Prevent WDT reset during long transmissions

            size_t chunk = std::min(total - sent, static_cast<size_t>(1024));
            size_t written = http.write(data + sent, chunk);

            if (written == 0) {
                yield();
                if (++stall_count > kMaxStallIterations) {
                    http.stop();
                    return {0, "Error: Write Stalled", {}};
                }
                yield();
                ::delay(5);
                continue;
            }

            stall_count = 0;
            sent += written;
        }
    }

    http.endRequest();

    // Read response with yield() to prevent WDT reset
    yield();
    int status = http.responseStatusCode();

    // Read response body with bulk reads (avoids O(n^2) char-by-char Arduino String reallocation).
    // Wait on connected() || available() — available() alone returns 0 between TCP packets,
    // causing premature exit on multi-packet LLM responses.
    std::string resp_body;
    int content_length = http.contentLength();
    if (content_length > 0) {
        resp_body.reserve(static_cast<size_t>(content_length));
    }
    char buf[256];
    unsigned long last_read_ms = millis();
    while (http.connected() || http.available()) {
        yield();
        if (http.available()) {
            int bytes_read = http.readBytes(buf, std::min(static_cast<int>(sizeof(buf)), http.available()));
            if (bytes_read > 0) {
                resp_body.append(buf, static_cast<size_t>(bytes_read));
                last_read_ms = millis();
            }
        } else {
            // Connection open but no data yet — wait for next packet
            if (millis() - last_read_ms > timeout_ms_) {
                break;
            }
            ::delay(5);
        }
    }

    // Clean up
    http.stop();
    client_->stop();

    // Build response
    HttpResponse response;
    response.status_code = status;
    response.body = std::move(resp_body);

    // Convert negative status codes to 503
    if (status <= 0) {
        response.status_code = 503;
        response.body = "Network Error";
    }

    return response;
}

}  // namespace arduino
}  // namespace hal
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_ARDUINO && FORESTHUB_ENABLE_NETWORK
