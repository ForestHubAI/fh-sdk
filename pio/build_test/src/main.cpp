// PlatformIO build verification + memory measurement.
// Constructs the Arduino platform and touches each subsystem so the linker
// keeps the code and memory measurements are realistic.

#include <Arduino.h>

#include "foresthub/platform/platform.hpp"
#include "platform/arduino/platform.hpp"

void setup() {
    using namespace foresthub::platform;

    arduino::ArduinoConfig config;
    config.baud_rate = 115200;
    auto platform = std::make_shared<arduino::ArduinoPlatform>(config);

    // Console + Time (always available)
    platform->console->Begin();
    platform->console->Printf("Booting...\n");
    platform->time->Delay(10);
    unsigned long ms = platform->time->GetMillis();
    platform->console->Printf("Uptime: %lu ms\n", ms);

#ifdef FORESTHUB_ENABLE_NETWORK
    platform->network->Connect();
    platform->console->Printf("IP: %s\n", platform->network->GetLocalIp().c_str());
#endif

#ifdef FORESTHUB_ENABLE_CRYPTO
    bool tls_ok = platform->crypto->IsAvailable();
    platform->console->Printf("TLS: %s\n", tls_ok ? "yes" : "no");
#endif

#ifdef FORESTHUB_ENABLE_GPIO
    platform->gpio->SetPinMode(2, foresthub::platform::PinMode::kOutput);
    platform->gpio->DigitalWrite(2, 1);
    platform->console->Printf("GPIO: pin 2 HIGH\n");
#endif

#if defined(FORESTHUB_ENABLE_NETWORK) && defined(FORESTHUB_ENABLE_CRYPTO)
    foresthub::core::HttpClientConfig http_cfg;
    http_cfg.host = "example.com";
    auto http = platform->CreateHttpClient(http_cfg);
    platform->console->Printf("HTTP client: %s\n", http ? "ok" : "null");
#endif

    platform->console->Printf("Done.\n");
}

void loop() {
    delay(10000);
}
