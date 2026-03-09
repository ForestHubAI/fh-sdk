#include "foresthub/rag/remote/retriever.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "foresthub/config/config.hpp"
#include "foresthub/rag/types.hpp"
#include "foresthub/util/json.hpp"
#include "mocks/mock_http_client.hpp"

namespace foresthub {
namespace rag {
namespace remote {
namespace {

using core::HttpResponse;
using json = nlohmann::json;

struct TestFixture {
    std::shared_ptr<tests::MockHttpClient> mock_http = std::make_shared<tests::MockHttpClient>();
    config::ProviderConfig cfg;

    TestFixture() {
        cfg.base_url = "https://api.example.com";
        cfg.api_key = "test-key-123";
    }

    RemoteRetriever MakeRetriever() { return RemoteRetriever(cfg, mock_http); }

    static std::string SingleResultBody() {
        json j = json::array({{{"chunkId", "c1"}, {"documentId", "d1"}, {"content", "Hello world"}, {"score", 0.95}}});
        return j.dump();
    }

    static std::string MultipleResultsBody() {
        json j = json::array({{{"chunkId", "c1"}, {"documentId", "d1"}, {"content", "First"}, {"score", 0.95}},
                              {{"chunkId", "c2"}, {"documentId", "d2"}, {"content", "Second"}, {"score", 0.80}},
                              {{"chunkId", "c3"}, {"documentId", "d3"}, {"content", "Third"}, {"score", 0.65}}});
        return j.dump();
    }
};

// --- Happy Path ---

TEST(RemoteRetriever, QuerySuccess) {
    TestFixture fixture;
    fixture.mock_http->post_responses.push_back({200, fixture.SingleResultBody(), {}});
    RemoteRetriever retriever = fixture.MakeRetriever();

    QueryRequest req;
    req.collection_id = "col-1";
    req.query = "test query";
    req.top_k = 3;

    auto resp = retriever.Query(req);
    ASSERT_NE(resp, nullptr);
    ASSERT_EQ(resp->results.size(), 1u);
    EXPECT_EQ(resp->results[0].chunk_id, "c1");
    EXPECT_EQ(resp->results[0].document_id, "d1");
    EXPECT_EQ(resp->results[0].content, "Hello world");
    EXPECT_DOUBLE_EQ(resp->results[0].score, 0.95);
}

TEST(RemoteRetriever, QueryMultipleResults) {
    TestFixture fixture;
    fixture.mock_http->post_responses.push_back({200, fixture.MultipleResultsBody(), {}});
    RemoteRetriever retriever = fixture.MakeRetriever();

    QueryRequest req;
    req.collection_id = "col-1";
    req.query = "test";

    auto resp = retriever.Query(req);
    ASSERT_NE(resp, nullptr);
    ASSERT_EQ(resp->results.size(), 3u);
    EXPECT_EQ(resp->results[0].content, "First");
    EXPECT_EQ(resp->results[1].content, "Second");
    EXPECT_EQ(resp->results[2].content, "Third");
}

TEST(RemoteRetriever, QueryEmptyResults) {
    TestFixture fixture;
    fixture.mock_http->post_responses.push_back({200, "[]", {}});
    RemoteRetriever retriever = fixture.MakeRetriever();

    QueryRequest req;
    req.collection_id = "col-1";
    req.query = "no match";

    auto resp = retriever.Query(req);
    ASSERT_NE(resp, nullptr);
    EXPECT_TRUE(resp->results.empty());
}

// --- HTTP Errors ---

TEST(RemoteRetriever, QueryHttpError404) {
    TestFixture fixture;
    fixture.mock_http->post_responses.push_back({404, "not found", {}});
    RemoteRetriever retriever = fixture.MakeRetriever();

    QueryRequest req;
    req.collection_id = "col-1";
    req.query = "test";

    EXPECT_EQ(retriever.Query(req), nullptr);
}

TEST(RemoteRetriever, QueryHttpError500) {
    TestFixture fixture;
    fixture.mock_http->post_responses.push_back({500, "internal error", {}});
    fixture.mock_http->post_responses.push_back({500, "internal error", {}});
    RemoteRetriever retriever = fixture.MakeRetriever();

    QueryRequest req;
    req.collection_id = "col-1";
    req.query = "test";

    EXPECT_EQ(retriever.Query(req), nullptr);
    EXPECT_EQ(fixture.mock_http->post_call_count, 2);
}

// --- JSON Errors ---

TEST(RemoteRetriever, QueryJsonParseError) {
    TestFixture fixture;
    fixture.mock_http->post_responses.push_back({200, "not json{{{", {}});
    RemoteRetriever retriever = fixture.MakeRetriever();

    QueryRequest req;
    req.collection_id = "col-1";
    req.query = "test";

    EXPECT_EQ(retriever.Query(req), nullptr);
}

TEST(RemoteRetriever, QueryJsonNotArray) {
    TestFixture fixture;
    fixture.mock_http->post_responses.push_back({200, R"({"error": "unexpected"})", {}});
    RemoteRetriever retriever = fixture.MakeRetriever();

    QueryRequest req;
    req.collection_id = "col-1";
    req.query = "test";

    EXPECT_EQ(retriever.Query(req), nullptr);
}

TEST(RemoteRetriever, QueryJsonArrayElementNotObject) {
    TestFixture fixture;
    fixture.mock_http->post_responses.push_back({200, R"([1, 2, 3])", {}});
    RemoteRetriever retriever = fixture.MakeRetriever();

    QueryRequest req;
    req.collection_id = "col-1";
    req.query = "test";

    EXPECT_EQ(retriever.Query(req), nullptr);
}

// --- Retry Logic ---

TEST(RemoteRetriever, QueryRetry429) {
    TestFixture fixture;
    fixture.mock_http->post_responses.push_back({429, "rate limited", {}});
    fixture.mock_http->post_responses.push_back({200, fixture.SingleResultBody(), {}});
    RemoteRetriever retriever = fixture.MakeRetriever();

    QueryRequest req;
    req.collection_id = "col-1";
    req.query = "test";

    auto resp = retriever.Query(req);
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->results.size(), 1u);
    EXPECT_EQ(fixture.mock_http->post_call_count, 2);
}

TEST(RemoteRetriever, QueryNoRetryOn400) {
    TestFixture fixture;
    fixture.mock_http->post_responses.push_back({400, "bad request", {}});
    RemoteRetriever retriever = fixture.MakeRetriever();

    QueryRequest req;
    req.collection_id = "col-1";
    req.query = "test";

    EXPECT_EQ(retriever.Query(req), nullptr);
    EXPECT_EQ(fixture.mock_http->post_call_count, 1);
}

// --- Request Serialization ---

TEST(RemoteRetriever, QueryRequestSerialization) {
    TestFixture fixture;
    fixture.mock_http->post_responses.push_back({200, "[]", {}});
    RemoteRetriever retriever = fixture.MakeRetriever();

    QueryRequest req;
    req.collection_id = "my-collection";
    req.query = "search text";
    req.top_k = 7;

    retriever.Query(req);

    json sent = json::parse(fixture.mock_http->last_body, nullptr, false);
    ASSERT_FALSE(sent.is_discarded());
    EXPECT_EQ(sent.value("collectionId", ""), "my-collection");
    EXPECT_EQ(sent.value("query", ""), "search text");
    EXPECT_EQ(sent.value("topK", 0), 7);
}

TEST(RemoteRetriever, QuerySendsDeviceKeyHeader) {
    TestFixture fixture;
    fixture.mock_http->post_responses.push_back({200, "[]", {}});
    RemoteRetriever retriever = fixture.MakeRetriever();

    QueryRequest req;
    req.collection_id = "col-1";
    req.query = "test";

    retriever.Query(req);

    EXPECT_EQ(fixture.mock_http->last_headers.at("Device-Key"), "test-key-123");
    EXPECT_EQ(fixture.mock_http->last_headers.at("Content-Type"), "application/json");
}

// --- Constructor ---

TEST(RemoteRetriever, ConstructorStripsTrailingSlash) {
    TestFixture fixture;
    fixture.cfg.base_url = "https://api.example.com/";
    fixture.mock_http->post_responses.push_back({200, "[]", {}});
    RemoteRetriever retriever = fixture.MakeRetriever();

    QueryRequest req;
    req.collection_id = "col-1";
    req.query = "test";

    retriever.Query(req);

    EXPECT_EQ(fixture.mock_http->last_url, "https://api.example.com/rag/query");
}

// --- FormatContext ---

TEST(FormatContext, FormatsResultsAsXml) {
    std::vector<QueryResult> results = {
        {"c1", "d1", "First chunk", 0.95},
        {"c2", "d2", "Second chunk", 0.80},
    };

    std::string output = FormatContext(results);

    std::string expected =
        "<context>\n"
        "<source id=\"1\">First chunk</source>\n"
        "<source id=\"2\">Second chunk</source>\n"
        "</context>";

    EXPECT_EQ(output, expected);
}

TEST(FormatContext, EmptyResultsReturnsEmpty) {
    std::vector<QueryResult> results;
    EXPECT_EQ(FormatContext(results), "");
}

}  // namespace
}  // namespace remote
}  // namespace rag
}  // namespace foresthub
