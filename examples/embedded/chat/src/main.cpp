// =============================================================================
// ForestHub Embedded Chat Example
// =============================================================================
// Simple chat request on Arduino (ESP32, Portenta H7).
// Sends a single prompt to the LLM and prints the response via the console HAL.
//
// Before compiling, edit env.cpp with your WiFi credentials and API key.
// =============================================================================

#include <Arduino.h>

#include "env.hpp"
#include "foresthub/client.hpp"
#include "foresthub/config/config.hpp"
#include "foresthub/core/input.hpp"
#include "foresthub/core/types.hpp"
#include "foresthub/platform/platform.hpp"

static std::shared_ptr<foresthub::platform::PlatformContext> platform;

void setup() {
    // 1. Create platform context (WiFi, Serial, NTP, TLS)
    foresthub::platform::PlatformConfig config;
    config.network.ssid = kWifiSsid;
    config.network.password = kWifiPassword;
    platform = foresthub::platform::CreatePlatform(config);
    if (!platform) {
        while (true) {
        }
    }

    // 2. Initialize console
    platform->console->Begin();
    platform->console->Printf("=== ForestHub Embedded Chat ===\n\n");
    platform->console->Flush();

    // 3. Connect network (retry up to 3 times — Portenta H7 WiFi is slow to initialize)
    std::string net_err;
    for (int attempt = 1; attempt <= 3; ++attempt) {
        net_err = platform->network->Connect();
        if (net_err.empty()) break;
        platform->console->Printf("[WARN] Network attempt %d/3: %s\n", attempt, net_err.c_str());
        if (attempt < 3) {
            platform->console->Printf("[INFO] Retrying in 2s...\n");
            platform->console->Flush();
            platform->time->Delay(2000);
        }
    }
    if (!net_err.empty()) {
        platform->console->Printf("[FATAL] Network: %s\n", net_err.c_str());
        platform->console->Flush();
        while (true) {
            platform->time->Delay(1000);
        }
    }
    platform->console->Printf("[OK] Network connected (%s)\n", platform->network->GetLocalIp().c_str());

    // 4. Synchronize time
    std::string time_err = platform->time->SyncTime();
    if (!time_err.empty()) {
        platform->console->Printf("[FATAL] Time sync: %s\n", time_err.c_str());
        platform->console->Flush();
        while (true) {
            platform->time->Delay(1000);
        }
    }
    platform->console->Printf("[OK] Time synced\n\n");

    // 5. Create HTTP client via HAL
    foresthub::platform::HttpClientConfig http_cfg;
    http_cfg.host = "fh-backend-368736749905.europe-west1.run.app";
    auto http_client = platform->CreateHttpClient(http_cfg);

    // 6. Configure ForestHub provider
    foresthub::config::ClientConfig cfg;
    foresthub::config::RemoteConfig remote_cfg;
    remote_cfg.base_url = "https://fh-backend-368736749905.europe-west1.run.app";
    remote_cfg.api_key = kForesthubApiKey;
    remote_cfg.supported_models = {"gpt-4.1", "gpt-4.1-mini", "gpt-4o", "gpt-4o-mini"};
    cfg.remote = remote_cfg;

    std::unique_ptr<foresthub::Client> client = foresthub::Client::Create(cfg, http_client);

    // 7. Health check
    platform->console->Printf("[INFO] Checking provider health...\n");
    std::string health_err = client->Health();
    if (!health_err.empty()) {
        platform->console->Printf("[ERROR] Health check failed: %s\n", health_err.c_str());
        platform->console->Flush();
        while (true) {
            platform->time->Delay(1000);
        }
    }
    platform->console->Printf("[OK] Provider healthy\n\n");

    // 8. Send chat request
    std::string model_name = "gpt-4.1-mini";
    std::string prompt = "Was ist die Hauptstadt von Frankreich? Antworte kurz.";

    platform->console->Printf("[INFO] Sending request...\n");
    platform->console->Printf("       Model: %s\n", model_name.c_str());
    platform->console->Printf("       Input: %s\n", prompt.c_str());

    auto input = std::make_shared<foresthub::core::InputString>(prompt);

    foresthub::core::ChatRequest req;
    req.model = model_name;
    req.input = input;
    req.WithSystemPrompt("You are a helpful geography assistant.");

    std::shared_ptr<foresthub::core::ChatResponse> response = client->Chat(req);

    // 9. Print result
    if (response) {
        platform->console->Printf("\n--------------------------------------------------\n");
        platform->console->Printf("LLM Response:\n");
        platform->console->Printf("%s\n", response->text.c_str());
        platform->console->Printf("--------------------------------------------------\n");
        platform->console->Printf("[INFO] Tokens used: %d\n", response->tokens_used);
    } else {
        platform->console->Printf("[ERROR] Request failed.\n");
    }

    platform->console->Printf("\n[DONE]\n");
    platform->console->Flush();
}

void loop() {
    // One-shot example — nothing to do in loop
    platform->time->Delay(10000);
}
