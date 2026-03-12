#include <Arduino.h>

#include "env.hpp"
#include "foresthub/platform/platform.hpp"

static std::shared_ptr<foresthub::platform::PlatformContext> platform;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void PrintLine() {
    platform->console->Printf("--------------------------------------------------\n");
}

static void PrintResult(const char* label, bool passed) {
    platform->console->Printf("  >> %s: %s\n\n", label, passed ? "PASS" : "FAIL");
}

// ---------------------------------------------------------------------------
// Test: Plain HTTP (port 80, no TLS)
// ---------------------------------------------------------------------------

static bool TestPlainHttp() {
    platform->console->Printf("[TEST 1] Plain HTTP (httpbin.org:80)\n");

    foresthub::platform::HttpClientConfig cfg;
    cfg.host = "httpbin.org";
    cfg.port = 80;
    cfg.use_tls = false;
    cfg.timeout_ms = 15000;

    auto http = platform->CreateHttpClient(cfg);
    if (!http) {
        platform->console->Printf("  Client creation: FAILED (nullptr)\n");
        PrintResult("HTTP", false);
        return false;
    }
    platform->console->Printf("  Client creation: OK\n");

    auto resp = http->Get("http://httpbin.org/get", {});
    platform->console->Printf("  Status code: %d\n", resp.status_code);

    if (resp.status_code == 200) {
        std::string preview = resp.body.substr(0, 120);
        platform->console->Printf("  Body preview: %s...\n", preview.c_str());
    } else {
        platform->console->Printf("  Body: %s\n", resp.body.empty() ? "(empty)" : resp.body.substr(0, 200).c_str());
    }

    bool passed = (resp.status_code == 200);
    PrintResult("HTTP", passed);
    return passed;
}

// ---------------------------------------------------------------------------
// Test: HTTPS (port 443, TLS)
// ---------------------------------------------------------------------------

static const char* kForesthubHost = "fh-backend-368736749905.europe-west1.run.app";

static bool TestHttps() {
#ifdef FORESTHUB_ENABLE_CRYPTO
    platform->console->Printf("[TEST 2] HTTPS (%s:443, TLS)\n", kForesthubHost);

    foresthub::platform::HttpClientConfig cfg;
    cfg.host = kForesthubHost;
    cfg.port = 443;
    cfg.use_tls = true;
    cfg.timeout_ms = 15000;

    auto http = platform->CreateHttpClient(cfg);
    if (!http) {
        platform->console->Printf("  Client creation: FAILED (nullptr)\n");
        PrintResult("HTTPS", false);
        return false;
    }
    platform->console->Printf("  Client creation: OK\n");

    // Use the health endpoint (no API key needed, returns 200 on success)
    std::string url = "https://";
    url += kForesthubHost;
    url += "/health";

    auto resp = http->Get(url, {});
    platform->console->Printf("  Status code: %d\n", resp.status_code);

    if (resp.status_code == 200) {
        std::string preview = resp.body.substr(0, 120);
        platform->console->Printf("  Body preview: %s...\n", preview.c_str());
    } else {
        platform->console->Printf("  Body: %s\n", resp.body.empty() ? "(empty)" : resp.body.substr(0, 200).c_str());
    }

    bool passed = (resp.status_code == 200);
    PrintResult("HTTPS", passed);
    return passed;
#else
    platform->console->Printf("[TEST 2] HTTPS — SKIPPED (FORESTHUB_ENABLE_CRYPTO not defined)\n");

    // Verify that CreateHttpClient returns nullptr for TLS when crypto is disabled
    foresthub::platform::HttpClientConfig cfg;
    cfg.host = kForesthubHost;
    cfg.port = 443;
    cfg.use_tls = true;
    cfg.timeout_ms = 15000;

    auto http = platform->CreateHttpClient(cfg);
    bool correct = (http == nullptr);
    platform->console->Printf("  CreateHttpClient(use_tls=true) returned: %s\n",
                              http ? "non-null (UNEXPECTED!)" : "nullptr (correct)");
    PrintResult("HTTPS nullptr check", correct);
    return correct;
#endif
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

void setup() {
    // 1. Create platform
    foresthub::platform::PlatformConfig config;
    config.network.ssid = kWifiSsid;
    config.network.password = kWifiPassword;
    platform = foresthub::platform::CreatePlatform(config);

    platform->console->Begin();
    platform->time->Delay(1000);

    PrintLine();
    platform->console->Printf("  ForestHub HTTP/HTTPS Test\n");
    PrintLine();
    platform->console->Printf("\n");

    // 2. Print build configuration
    platform->console->Printf("[CONFIG]\n");
    platform->console->Printf("  FORESTHUB_ENABLE_NETWORK: YES\n");
#ifdef FORESTHUB_ENABLE_CRYPTO
    platform->console->Printf("  FORESTHUB_ENABLE_CRYPTO:  YES\n");
#else
    platform->console->Printf("  FORESTHUB_ENABLE_CRYPTO:  NO\n");
#endif
    platform->console->Printf("  HTTP  test: httpbin.org\n");
    platform->console->Printf("  HTTPS test: %s\n\n", kForesthubHost);
    platform->console->Flush();

    // 3. Connect to WiFi
    platform->console->Printf("[WIFI] Connecting to '%s'...\n", kWifiSsid);
    platform->console->Flush();

    std::string net_err;
    for (int attempt = 1; attempt <= 3; ++attempt) {
        net_err = platform->network->Connect();
        if (net_err.empty()) break;
        platform->console->Printf("[WIFI] Attempt %d/3 failed: %s\n", attempt, net_err.c_str());
        if (attempt < 3) {
            platform->console->Printf("[WIFI] Retrying in 2s...\n");
            platform->console->Flush();
            platform->time->Delay(2000);
        }
    }

    if (!net_err.empty()) {
        platform->console->Printf("\n[FATAL] WiFi connection failed. Cannot run tests.\n");
        platform->console->Flush();
        while (true) {
            platform->time->Delay(10000);
        }
    }

    platform->console->Printf("[WIFI] Connected — IP: %s\n\n", platform->network->GetLocalIp().c_str());
    platform->console->Flush();

    // 4. Run tests
    bool http_pass = TestPlainHttp();
    platform->console->Flush();

    bool https_pass = TestHttps();
    platform->console->Flush();

    // 5. Summary
    PrintLine();
    platform->console->Printf("  RESULTS\n");
    PrintLine();
    platform->console->Printf("\n");
    platform->console->Printf("  HTTP  (plain):  %s\n", http_pass ? "PASS" : "FAIL");
#ifdef FORESTHUB_ENABLE_CRYPTO
    platform->console->Printf("  HTTPS (TLS):    %s\n", https_pass ? "PASS" : "FAIL");
#else
    platform->console->Printf("  HTTPS (nullptr): %s\n", https_pass ? "PASS" : "FAIL");
#endif
    platform->console->Printf("\n");

    bool all_pass = http_pass && https_pass;
    platform->console->Printf("  OVERALL: %s\n\n", all_pass ? "ALL PASSED" : "SOME FAILED");
    platform->console->Flush();
}

void loop() {
    platform->time->Delay(10000);
}
