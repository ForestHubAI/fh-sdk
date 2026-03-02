#ifndef FORESTHUB_PLATFORM_PC_HTTP_CLIENT_HPP
#define FORESTHUB_PLATFORM_PC_HTTP_CLIENT_HPP

#include <string>

#include "foresthub/core/http_client.hpp"

namespace foresthub {
namespace platform {
namespace pc {

/// HTTP client implementation for PC using CPR/libcurl.
class PcHttpClient : public core::HttpClient {
public:
    /// Construct with configurable request timeout.
    explicit PcHttpClient(int timeout_ms = 30000);

    /// Delegates to cpr::Get with timeout.
    core::HttpResponse Get(const std::string& url, const Headers& headers) override;
    /// Delegates to cpr::Post with timeout.
    core::HttpResponse Post(const std::string& url, const Headers& headers, const std::string& body) override;
    /// Sleeps via std::this_thread::sleep_for.
    void Delay(unsigned long ms) override;

private:
    int timeout_ms_;  ///< Request timeout in milliseconds.
};

}  // namespace pc
}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_PC_HTTP_CLIENT_HPP
