#include "foresthub/platform/platform.hpp"

#include <gtest/gtest.h>

#include <ctime>
#include <memory>
#include <string>

namespace foresthub {
namespace platform {
namespace {

// --- DOC-06/07: CreatePlatform factory ---

TEST(PlatformTest, CreatePlatformReturnsValidContext) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    ASSERT_NE(ctx, nullptr);
}

TEST(PlatformTest, AllSubsystemsNonNull) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    ASSERT_NE(ctx, nullptr);
    EXPECT_NE(ctx->network, nullptr);
    EXPECT_NE(ctx->console, nullptr);
    EXPECT_NE(ctx->time, nullptr);
    EXPECT_NE(ctx->crypto, nullptr);
    EXPECT_NE(ctx->gpio, nullptr);
}

// --- DOC-08: Console subsystem ---

TEST(PlatformTest, ConsoleBegin) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    // Begin should not crash. ConsoleInterface::Begin returns void.
    ctx->console->Begin();
}

TEST(PlatformTest, ConsoleWriteAndFlush) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    ctx->console->Begin();
    ctx->console->Write("test output");
    ctx->console->Flush();
}

// --- DOC-09: Network mock ---

TEST(PlatformTest, NetworkMockConnected) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    std::string err = ctx->network->Connect();
    EXPECT_TRUE(err.empty()) << "Connect() failed: " << err;
    EXPECT_EQ(ctx->network->GetStatus(), NetworkStatus::kConnected);
}

TEST(PlatformTest, NetworkGetLocalIp) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    ctx->network->Connect();
    std::string ip = ctx->network->GetLocalIp();
    EXPECT_FALSE(ip.empty());
}

// --- Time subsystem ---

TEST(PlatformTest, TimeGetMillis) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    // GetMillis() returns elapsed since construction. A brief delay ensures non-zero.
    ctx->time->Delay(1);
    unsigned long ms = ctx->time->GetMillis();
    EXPECT_GT(ms, 0u);
}

TEST(PlatformTest, TimeMonotonicity) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    unsigned long first = ctx->time->GetMillis();
    unsigned long second = ctx->time->GetMillis();
    EXPECT_GE(second, first);
}

TEST(PlatformTest, TimeDelay) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    // Delay(10) should not crash.
    ctx->time->Delay(10);
}

// --- Crypto subsystem ---

TEST(PlatformTest, CryptoIsAvailable) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    EXPECT_TRUE(ctx->crypto->IsAvailable());
}

TEST(PlatformTest, CryptoGetGtsRootCerts) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    // PC returns nullptr (uses system cert store). Arduino returns embedded GTS certs.
    // Just verify the call doesn't crash.
    ctx->crypto->GetGtsRootCerts();
}

// --- HttpClient creation ---

TEST(PlatformTest, CreateHttpClient) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    HttpClientConfig http_cfg;
    http_cfg.host = "example.com";
    http_cfg.timeout_ms = 5000;
    std::shared_ptr<core::HttpClient> http = ctx->CreateHttpClient(http_cfg);
    EXPECT_NE(http, nullptr);
}

// --- Additional Time subsystem tests ---

TEST(PlatformTest, TimeSyncTime) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    std::string err = ctx->time->SyncTime();
    EXPECT_TRUE(err.empty()) << "SyncTime() failed: " << err;
}

TEST(PlatformTest, TimeGetEpochTime) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    unsigned long epoch = ctx->time->GetEpochTime();
    EXPECT_GT(epoch, 0u);
}

TEST(PlatformTest, TimeIsSynced) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    EXPECT_TRUE(ctx->time->IsSynced(0));
}

// --- Additional Network tests ---

TEST(PlatformTest, NetworkDisconnect) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    ctx->network->Connect();
    ctx->network->Disconnect();
    // No crash expected. Status may still be kConnected (mock).
}

TEST(PlatformTest, NetworkSignalStrength) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    int strength = ctx->network->GetSignalStrength();
    EXPECT_EQ(strength, 0);  // PC mock returns 0.
}

// --- Additional Crypto test ---

TEST(PlatformTest, CryptoCreateTlsClient) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    // PC returns nullptr (TLS handled by CPR/libcurl).
    std::shared_ptr<TLSClientWrapper> tls = ctx->crypto->CreateTlsClient(nullptr, 5000);
    EXPECT_EQ(tls, nullptr);
}

// --- CreatePlatform with config values ---

TEST(PlatformTest, CreatePlatformWithConfig) {
    PlatformConfig config;
    config.network.ssid = "TestSSID";
    config.network.password = "TestPassword";
    std::shared_ptr<PlatformContext> ctx = CreatePlatform(config);
    ASSERT_NE(ctx, nullptr);
    EXPECT_NE(ctx->network, nullptr);
    EXPECT_NE(ctx->console, nullptr);
    EXPECT_NE(ctx->time, nullptr);
    EXPECT_NE(ctx->crypto, nullptr);
    EXPECT_NE(ctx->gpio, nullptr);
}

// --- ENABLE macros ---

TEST(PlatformTest, EnableMacrosDefinedOnPC) {
#ifdef FORESTHUB_ENABLE_NETWORK
    SUCCEED();
#else
    FAIL() << "FORESTHUB_ENABLE_NETWORK should be defined on PC";
#endif
#ifdef FORESTHUB_ENABLE_CRYPTO
    SUCCEED();
#else
    FAIL() << "FORESTHUB_ENABLE_CRYPTO should be defined on PC";
#endif
#ifdef FORESTHUB_ENABLE_GPIO
    SUCCEED();
#else
    FAIL() << "FORESTHUB_ENABLE_GPIO should be defined on PC";
#endif
}

TEST(PlatformTest, CreateHttpClientWithAllSubsystems) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    HttpClientConfig http_cfg;
    http_cfg.host = "example.com";
    std::shared_ptr<core::HttpClient> http = ctx->CreateHttpClient(http_cfg);
    EXPECT_NE(http, nullptr);
    EXPECT_NE(ctx->network, nullptr);
    EXPECT_NE(ctx->crypto, nullptr);
}

// --- Timezone support ---

TEST(PlatformTest, DefaultOffsetIsZero) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    EXPECT_EQ(ctx->time->gmt_offset_sec(), 0);
}

TEST(PlatformTest, SetOffsetGmtOnly) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    ctx->time->SetOffset(3600, 0);
    EXPECT_EQ(ctx->time->gmt_offset_sec(), 3600);
}

TEST(PlatformTest, SetOffsetGmtPlusDaylight) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    ctx->time->SetOffset(3600, 3600);
    EXPECT_EQ(ctx->time->gmt_offset_sec(), 7200);
}

TEST(PlatformTest, SyncTimeStoresOffset) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    TimeConfig config;
    config.gmt_offset_sec = 7200;
    config.daylight_offset_sec = 3600;
    ctx->time->SyncTime(config);
    EXPECT_EQ(ctx->time->gmt_offset_sec(), 10800);
}

TEST(PlatformTest, GetLocalTimeAppliesOffset) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    ctx->time->SetOffset(3600, 0);  // CET: UTC+1

    struct tm local = {};
    ctx->time->GetLocalTime(local);

    // Compare with gmtime(epoch + 3600) to verify offset is applied.
    time_t epoch = static_cast<time_t>(ctx->time->GetEpochTime());
    time_t adjusted = epoch + 3600;
    struct tm* expected = gmtime(&adjusted);
    ASSERT_NE(expected, nullptr);

    EXPECT_EQ(local.tm_hour, expected->tm_hour);
    EXPECT_EQ(local.tm_min, expected->tm_min);
}

TEST(PlatformTest, GetLocalTimeUtcMatchesGmtime) {
    std::shared_ptr<PlatformContext> ctx = CreatePlatform();
    // No offset set (default 0) — local time should match UTC.
    struct tm local = {};
    ctx->time->GetLocalTime(local);

    time_t epoch = static_cast<time_t>(ctx->time->GetEpochTime());
    struct tm* utc = gmtime(&epoch);
    ASSERT_NE(utc, nullptr);

    EXPECT_EQ(local.tm_hour, utc->tm_hour);
    EXPECT_EQ(local.tm_min, utc->tm_min);
}

TEST(PlatformTest, PlatformConfigNoEnableGpioField) {
    PlatformConfig config;
    config.network.ssid = "test";
    config.baud_rate = 9600;
    // If this compiles, enable_gpio field does not exist.
    std::shared_ptr<PlatformContext> ctx = CreatePlatform(config);
    ASSERT_NE(ctx, nullptr);
    EXPECT_NE(ctx->gpio, nullptr);
}

}  // namespace
}  // namespace platform
}  // namespace foresthub
