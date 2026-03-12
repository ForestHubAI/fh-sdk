#ifndef FORESTHUB_CORE_HTTP_CLIENT_HPP
#define FORESTHUB_CORE_HTTP_CLIENT_HPP

/// @file
/// Abstract HTTP client interface and response type.

#include <map>
#include <string>

namespace foresthub {
namespace core {

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

}  // namespace core
}  // namespace foresthub

#endif  // FORESTHUB_CORE_HTTP_CLIENT_HPP
