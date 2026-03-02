#ifndef FORESTHUB_PLATFORM_PC_PLATFORM_HPP
#define FORESTHUB_PLATFORM_PC_PLATFORM_HPP

#include "foresthub/platform/platform.hpp"

namespace foresthub {
namespace platform {
namespace pc {

/// PC platform context. Creates all HAL subsystems in the constructor.
class PcPlatformContext : public PlatformContext {
public:
    /// Initialize PC platform with all subsystems.
    explicit PcPlatformContext(const PlatformConfig& config = {});

    /// Ignores host/port/use_tls; CPR handles routing per request.
    std::shared_ptr<core::HttpClient> CreateHttpClient(const HttpClientConfig& config) override;
};

}  // namespace pc
}  // namespace platform
}  // namespace foresthub

#endif  // FORESTHUB_PLATFORM_PC_PLATFORM_HPP
