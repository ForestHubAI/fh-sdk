// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 ForestHub. All rights reserved.
// For commercial licensing, visit https://github.com/ForestHubAI/fh-sdk

#include "foresthub/hal/platform.hpp"

#include <gtest/gtest.h>

#include <ctime>
#include <memory>
#include <string>

#include "hal/pc/platform.hpp"

namespace foresthub {
namespace hal {
namespace {

// --- PcPlatform construction ---

TEST(PlatformTest, CreatePlatformReturnsValidContext) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    ASSERT_NE(ctx, nullptr);
}

TEST(PlatformTest, AllSubsystemsNonNull) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    EXPECT_NE(ctx->network, nullptr);
    EXPECT_NE(ctx->console, nullptr);
    EXPECT_NE(ctx->time, nullptr);
    EXPECT_NE(ctx->crypto, nullptr);
    EXPECT_NE(ctx->gpio, nullptr);
}

// --- DOC-08: Console subsystem ---

TEST(PlatformTest, ConsoleBegin) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    // Begin should not crash. Console::Begin returns void.
    ctx->console->Begin();
}

TEST(PlatformTest, ConsoleWriteAndFlush) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    ctx->console->Begin();
    ctx->console->Write("test output");
    ctx->console->Flush();
}

// --- DOC-09: Network mock ---

TEST(PlatformTest, NetworkMockConnected) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    std::string err = ctx->network->Connect();
    EXPECT_TRUE(err.empty()) << "Connect() failed: " << err;
    EXPECT_EQ(ctx->network->GetStatus(), NetworkStatus::kConnected);
}

TEST(PlatformTest, NetworkGetLocalIp) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    ctx->network->Connect();
    std::string ip = ctx->network->GetLocalIp();
    EXPECT_FALSE(ip.empty());
}

// --- Time subsystem ---

TEST(PlatformTest, TimeGetMillis) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    // GetMillis() returns elapsed since construction. A brief delay ensures non-zero.
    ctx->time->Delay(1);
    unsigned long ms = ctx->time->GetMillis();
    EXPECT_GT(ms, 0u);
}

TEST(PlatformTest, TimeMonotonicity) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    unsigned long first = ctx->time->GetMillis();
    unsigned long second = ctx->time->GetMillis();
    EXPECT_GE(second, first);
}

TEST(PlatformTest, TimeDelay) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    // Delay(10) should not crash.
    ctx->time->Delay(10);
}

// --- Crypto subsystem ---

TEST(PlatformTest, CryptoIsAvailable) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    EXPECT_TRUE(ctx->crypto->IsAvailable());
}

TEST(PlatformTest, CryptoGetGtsRootCerts) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    // PC returns nullptr (uses system cert store). Arduino returns embedded GTS certs.
    // Just verify the call doesn't crash.
    ctx->crypto->GetGtsRootCerts();
}

// --- HttpClient creation ---

TEST(PlatformTest, CreateHttpClient) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    hal::HttpClientConfig http_cfg;
    http_cfg.host = "example.com";
    http_cfg.timeout_ms = 5000;
    std::shared_ptr<hal::HttpClient> http = ctx->CreateHttpClient(http_cfg);
    EXPECT_NE(http, nullptr);
}

// --- Additional Time subsystem tests ---

TEST(PlatformTest, TimeSyncTime) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    std::string err = ctx->time->SyncTime();
    EXPECT_TRUE(err.empty()) << "SyncTime() failed: " << err;
}

TEST(PlatformTest, TimeGetEpochTime) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    unsigned long epoch = ctx->time->GetEpochTime();
    EXPECT_GT(epoch, 0u);
}

TEST(PlatformTest, TimeIsSynced) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    EXPECT_TRUE(ctx->time->IsSynced(0));
}

// --- Additional Network tests ---

TEST(PlatformTest, NetworkDisconnect) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    ctx->network->Connect();
    ctx->network->Disconnect();
    // No crash expected. Status may still be kConnected (mock).
}

TEST(PlatformTest, NetworkSignalStrength) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    int strength = ctx->network->GetSignalStrength();
    EXPECT_EQ(strength, 0);  // PC mock returns 0.
}

// --- Additional Crypto test ---

TEST(PlatformTest, CryptoCreateTlsClient) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    // PC returns nullptr (TLS handled by CPR/libcurl).
    std::shared_ptr<TLSClientWrapper> tls = ctx->crypto->CreateTlsClient(nullptr, 5000);
    EXPECT_EQ(tls, nullptr);
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
    auto ctx = std::make_shared<pc::PcPlatform>();
    hal::HttpClientConfig http_cfg;
    http_cfg.host = "example.com";
    std::shared_ptr<hal::HttpClient> http = ctx->CreateHttpClient(http_cfg);
    EXPECT_NE(http, nullptr);
    EXPECT_NE(ctx->network, nullptr);
    EXPECT_NE(ctx->crypto, nullptr);
}

// --- Timezone support ---

TEST(PlatformTest, DefaultOffsetIsZero) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    EXPECT_EQ(ctx->time->utc_offset_sec(), 0);
}

TEST(PlatformTest, SetOffsetGmtOnly) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    ctx->time->SetOffset(3600, 0);
    EXPECT_EQ(ctx->time->utc_offset_sec(), 3600);
}

TEST(PlatformTest, SetOffsetGmtPlusDaylight) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    ctx->time->SetOffset(3600, 3600);
    EXPECT_EQ(ctx->time->utc_offset_sec(), 7200);
}

TEST(PlatformTest, SyncTimeStoresOffset) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    TimeConfig config;
    config.std_offset_sec = 7200;
    config.dst_offset_sec = 3600;
    ctx->time->SyncTime(config);
    EXPECT_EQ(ctx->time->utc_offset_sec(), 10800);
}

TEST(PlatformTest, GetLocalTimeAppliesOffset) {
    auto ctx = std::make_shared<pc::PcPlatform>();
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
    auto ctx = std::make_shared<pc::PcPlatform>();
    // No offset set (default 0) — local time should match UTC.
    struct tm local = {};
    ctx->time->GetLocalTime(local);

    time_t epoch = static_cast<time_t>(ctx->time->GetEpochTime());
    struct tm* utc = gmtime(&epoch);
    ASSERT_NE(utc, nullptr);

    EXPECT_EQ(local.tm_hour, utc->tm_hour);
    EXPECT_EQ(local.tm_min, utc->tm_min);
}

TEST(PlatformTest, GetLocalTimeIsDstWhenActive) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    ctx->time->SetOffset(3600, 3600);  // CEST: UTC+2, DST active

    struct tm local = {};
    ctx->time->GetLocalTime(local);
    EXPECT_EQ(local.tm_isdst, 1);
}

TEST(PlatformTest, GetLocalTimeIsDstWhenInactive) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    ctx->time->SetOffset(3600, 0);  // CET: UTC+1, no DST

    struct tm local = {};
    ctx->time->GetLocalTime(local);
    EXPECT_EQ(local.tm_isdst, 0);
}

TEST(PlatformTest, GetLocalEpochMatchesManualCalc) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    ctx->time->SetOffset(3600, 3600);  // CEST: UTC+2

    unsigned long expected = ctx->time->GetEpochTime() + ctx->time->utc_offset_sec();
    EXPECT_EQ(ctx->time->GetLocalEpoch(), expected);
}

TEST(PlatformTest, GetLocalEpochDefaultNoOffset) {
    auto ctx = std::make_shared<pc::PcPlatform>();
    // No offset — GetLocalEpoch should equal GetEpochTime.
    EXPECT_EQ(ctx->time->GetLocalEpoch(), ctx->time->GetEpochTime());
}

}  // namespace
}  // namespace hal
}  // namespace foresthub
