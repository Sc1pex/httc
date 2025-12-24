#include <httc/request_parser.h>
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

const std::size_t MAX_HEADER_SIZE = 16 * 1024;
const std::size_t MAX_BODY_SIZE = 10 * 1024 * 1024;

TEST_CASE("Parse request line") {
    httc::RequestParser parser{ MAX_HEADER_SIZE, MAX_BODY_SIZE };

    SECTION("Valid request line") {
        std::string_view message = "GET /index.html HTTP/1.1\r\n\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        REQUIRE(req.method == "GET");
        REQUIRE(req.uri.to_string() == "/index.html");
    }

    SECTION("Valid request line with url encoding") {
        std::string_view message = "GET /abc%20def HTTP/1.1\r\n\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        REQUIRE(req.method == "GET");
        REQUIRE(req.uri.to_string() == "/abc def");
    }

    SECTION("Invalid HTTP version") {
        std::string_view message = "GET /index.html HTTP/2.0\r\n\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_REQUEST_LINE);
    }

    SECTION("Request line with leading CRLF") {
        std::string_view message = "\r\n\r\nGET /index.html HTTP/1.1\r\n\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        REQUIRE(req.method == "GET");
        REQUIRE(req.uri.to_string() == "/index.html");
    }

    SECTION("Invalid method token") {
        std::string_view message = "GE==T /index.html HTTP/1.1\r\n\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_REQUEST_LINE);
    }

    SECTION("Invalid request line 1") {
        std::string_view message = "INVALID_REQUEST_LINE\r\n\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_REQUEST_LINE);
    }

    SECTION("Invalid request line 2") {
        std::string_view message = "GET /index.html\r\n\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_REQUEST_LINE);
    }

    SECTION("Invalid request line 3") {
        std::string_view message = "GET /index.html HTTP/1.1 EXTRA\r\n\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_REQUEST_LINE);
    }
}

TEST_CASE("Parse query parameters") {
    httc::RequestParser parser{ MAX_HEADER_SIZE, MAX_BODY_SIZE };

    SECTION("Valid query parameters") {
        std::string_view message = "GET /search?q=test&page=1 HTTP/1.1\r\n\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        REQUIRE(req.method == "GET");
        auto q = req.uri.query_param("q");
        REQUIRE(q.has_value());
        REQUIRE(q.value() == "test");
        auto page = req.uri.query_param("page");
        REQUIRE(page.has_value());
        REQUIRE(page.value() == "1");
    }

    SECTION("Empty query parameter value") {
        std::string_view message = "GET /search?q=&p= HTTP/1.1\r\n\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        REQUIRE(req.method == "GET");
        auto q = req.uri.query_param("q");
        REQUIRE(q.has_value());
        REQUIRE(q.value() == "");
        auto p = req.uri.query_param("p");
        REQUIRE(p.has_value());
        REQUIRE(p.value() == "");
    }
}

TEST_CASE("Parse headers") {
    httc::RequestParser parser{ MAX_HEADER_SIZE, MAX_BODY_SIZE };

    SECTION("Valid headers") {
        std::string_view message = "GET /index.html HTTP/1.1\r\n"
                                   "Host: example.com\r\n"
                                   "User-Agent: TestAgent/1.0\r\n"
                                   "\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        REQUIRE(req.method == "GET");
        REQUIRE(req.uri.to_string() == "/index.html");
        auto host = req.header("Host");
        REQUIRE(host.has_value());
        REQUIRE(host.value() == "example.com");
        auto user_agent = req.header("User-Agent");
        REQUIRE(user_agent.has_value());
        REQUIRE(user_agent.value() == "TestAgent/1.0");
    }

    SECTION("Invalid header name") {
        std::string_view message = "GET /index.html HTTP/1.1\r\n"
                                   "Inva lid-Header: value\r\n"
                                   "\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_HEADER);
    }

    SECTION("Invalid header value") {
        std::string_view message = "GET /index.html HTTP/1.1\r\n"
                                   "Valid-Header: value\x01\x02\x03\r\n"
                                   "\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_HEADER);
    }

    SECTION("Header whitespace handling 1") {
        std::string_view message = "GET /index.html HTTP/1.1\r\n"
                                   "X-Custom-Header:    value with spaces   \r\n"
                                   "\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        REQUIRE(req.method == "GET");
        REQUIRE(req.uri.to_string() == "/index.html");
        auto header = req.header("X-Custom-Header");
        REQUIRE(header.has_value());
        REQUIRE(header.value() == "value with spaces");
    }

    SECTION("Header whitespace handling 2") {
        std::string_view message = "GET /index.html HTTP/1.1\r\n"
                                   "X-Custom-Header:value with spaces\r\n"
                                   "\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        REQUIRE(req.method == "GET");
        REQUIRE(req.uri.to_string() == "/index.html");
        auto header = req.header("X-Custom-Header");
        REQUIRE(header.has_value());
        REQUIRE(header.value() == "value with spaces");
    }

    SECTION("Header with whitespace 3") {
        std::string_view message = "GET /index.html HTTP/1.1\r\n"
                                   "X-Custom-Header : value with spaces\r\n"
                                   "\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_HEADER);
    }

    SECTION("Missing colon in header") {
        std::string_view message = "GET /index.html HTTP/1.1\r\n"
                                   "Invalid-Header value\r\n"
                                   "\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_HEADER);
    }

    SECTION("Empty header name") {
        std::string_view message = "GET /index.html HTTP/1.1\r\n"
                                   ": value\r\n"
                                   "\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_HEADER);
    }

    SECTION("Empty header value") {
        std::string_view message = "GET /index.html HTTP/1.1\r\n"
                                   "X-Empty-Header: \r\n"
                                   "\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        REQUIRE(req.method == "GET");
        REQUIRE(req.uri.to_string() == "/index.html");
        auto header = req.header("X-Empty-Header");
        REQUIRE(header.has_value());
        REQUIRE(header.value() == "");
    }

    SECTION("Headers exceeding maximum size") {
        std::string large_header_value(MAX_HEADER_SIZE, 'a');
        std::string message = "GET /index.html HTTP/1.1\r\n"
                              "X-Large-Header: "
                              + large_header_value
                              + "\r\n"
                                "\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::HEADER_TOO_LARGE);
    }
}

TEST_CASE("Parse Content-length bodies") {
    httc::RequestParser parser{ MAX_HEADER_SIZE, MAX_BODY_SIZE };

    SECTION("Valid Content-Length body") {
        std::string_view message = "POST /submit HTTP/1.1\r\n"
                                   "Host: example.com\r\n"
                                   "Content-Length: 13\r\n"
                                   "\r\n"
                                   "Hello, World!";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        REQUIRE(req.method == "POST");
        REQUIRE(req.uri.to_string() == "/submit");
        auto content_length = req.header("Content-Length");
        REQUIRE(content_length.has_value());
        REQUIRE(content_length.value() == "13");
        REQUIRE(req.body == "Hello, World!");
    }

    SECTION("Invalid Content-Length value") {
        std::string_view message = "POST /submit HTTP/1.1\r\n"
                                   "Host: example.com\r\n"
                                   "Content-Length: invalid\r\n"
                                   "\r\n"
                                   "Hello, World!";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_HEADER);
    }

    SECTION("Content-Length exceeds maximum size") {
        std::string_view message = "POST /submit HTTP/1.1\r\n"
                                   "Host: example.com\r\n"
                                   "Content-Length: 10485771\r\n" // 10 MB + 1 byte
                                   "\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::CONTENT_TOO_LARGE);
    }
}

TEST_CASE("Parse chunked bodies") {
    httc::RequestParser parser{ MAX_HEADER_SIZE, MAX_BODY_SIZE };

    SECTION("Valid chunked body") {
        std::string_view message = "POST /submit HTTP/1.1\r\n"
                                   "Host: example.com\r\n"
                                   "Transfer-Encoding: chunked\r\n"
                                   "\r\n"
                                   "5\r\n"
                                   "Hello\r\n"
                                   "7\r\n"
                                   ", World\r\n"
                                   "0\r\n"
                                   "\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        REQUIRE(req.method == "POST");
        REQUIRE(req.uri.to_string() == "/submit");
        auto transfer_encoding = req.header("Transfer-Encoding");
        REQUIRE(transfer_encoding.has_value());
        REQUIRE(transfer_encoding.value() == "chunked");
        REQUIRE(req.body == "Hello, World");
    }

    SECTION("Invalid chunk size") {
        std::string_view message = "POST /submit HTTP/1.1\r\n"
                                   "Host: example.com\r\n"
                                   "Transfer-Encoding: chunked\r\n"
                                   "\r\n"
                                   "invalid\r\n"
                                   "Hello\r\n"
                                   "0\r\n"
                                   "\r\n";
        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_CHUNK_ENCODING);
    }

    SECTION("With trailers") {
        std::string_view message = "POST /submit HTTP/1.1\r\n"
                                   "Host: example.com\r\n"
                                   "Transfer-Encoding: chunked\r\n"
                                   "\r\n"
                                   "5\r\n"
                                   "Hello\r\n"
                                   "7\r\n"
                                   ", World\r\n"
                                   "0\r\n"
                                   "Trailer-Header: TrailerValue\r\n"
                                   "Other-Trailer: AnotherValue\r\n"
                                   "\r\n";

        auto result = parser.feed_data(message.data(), message.size());
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());

        const auto& req = result->value();
        auto trailers = req.trailers;

        auto trailer_header = trailers.get("Trailer-Header");
        REQUIRE(trailer_header.has_value());
        REQUIRE(trailer_header.value() == "TrailerValue");
        auto other_trailer = trailers.get("Other-Trailer");
        REQUIRE(other_trailer.has_value());
        REQUIRE(other_trailer.value() == "AnotherValue");
    }
}

TEST_CASE("Parse in multiple chunks") {
    httc::RequestParser parser{ MAX_HEADER_SIZE, MAX_BODY_SIZE };

    SECTION("Without body") {
        std::string_view message = "GET /index.html HTTP/1.1\r\n"
                                   "Host: example.com\r\n"
                                   "\r\n";
        std::optional<httc::RequestParser::ParseResult> result;
        for (int i = 0; i < message.size(); i++) {
            INFO("Feeding byte " << i);
            result = parser.feed_data(&message[i], 1);

            if (i < message.size() - 1) {
                REQUIRE(!result.has_value());
            } else {
                REQUIRE(result.has_value());
                REQUIRE(result->has_value());
                const auto& req = result->value();
                CHECK(req.method == "GET");
                CHECK(req.uri.to_string() == "/index.html");
                auto host = req.header("Host");
                CHECK(host.has_value());
                CHECK(host.value() == "example.com");
            }
        }
    }

    SECTION("With Content-Length body") {
        std::string_view message = "POST /submit HTTP/1.1\r\n"
                                   "Host: example.com\r\n"
                                   "Content-Length: 13\r\n"
                                   "\r\n"
                                   "Hello, World!";
        std::optional<httc::RequestParser::ParseResult> result;
        for (int i = 0; i < message.size(); i++) {
            INFO("Feeding byte " << i);
            result = parser.feed_data(&message[i], 1);

            if (i < message.size() - 1) {
                REQUIRE(!result.has_value());
            } else {
                REQUIRE(result.has_value());
                REQUIRE(result->has_value());
                const auto& req = result->value();
                CHECK(req.method == "POST");
                CHECK(req.uri.to_string() == "/submit");
                auto content_length = req.header("Content-Length");
                CHECK(content_length.has_value());
                CHECK(content_length.value() == "13");
                CHECK(req.body == "Hello, World!");
            }
        }
    }

    SECTION("With chunked body") {
        std::string_view message = "POST /submit HTTP/1.1\r\n"
                                   "Host: example.com\r\n"
                                   "Transfer-Encoding: chunked\r\n"
                                   "\r\n"
                                   "5\r\n"
                                   "Hello\r\n"
                                   "7\r\n"
                                   ", World\r\n"
                                   "0\r\n"
                                   "\r\n";
        std::optional<httc::RequestParser::ParseResult> result;
        for (int i = 0; i < message.size(); i++) {
            INFO("Feeding byte " << i);
            result = parser.feed_data(&message[i], 1);

            if (i < message.size() - 1) {
                REQUIRE(!result.has_value());
            } else {
                REQUIRE(result.has_value());
                REQUIRE(result->has_value());
                const auto& req = result->value();
                CHECK(req.method == "POST");
                CHECK(req.uri.to_string() == "/submit");
                auto transfer_encoding = req.header("Transfer-Encoding");
                CHECK(transfer_encoding.has_value());
                CHECK(transfer_encoding.value() == "chunked");
                CHECK(req.body == "Hello, World");
            }
        }
    }
}

TEST_CASE("Multiple requests") {
    httc::RequestParser parser{ MAX_HEADER_SIZE, MAX_BODY_SIZE };

    std::string_view message1 = "POST /abc HTTP/1.1\r\n"
                                "Host: example.com\r\n"
                                "Content-Length: 5\r\n"
                                "\r\n"
                                "HelloPOST";
    std::string_view message2 = " /submit HTTP/1.1\r\n"
                                "Host: example.com\r\n"
                                "Content-Length: 13\r\n"
                                "\r\n"
                                "Hello, World!";

    auto result1 = parser.feed_data(message1.data(), message1.size());
    REQUIRE(result1.has_value());
    REQUIRE(result1->has_value());
    const auto& req1 = result1->value();
    REQUIRE(req1.method == "POST");
    REQUIRE(req1.uri.to_string() == "/abc");
    auto content_length1 = req1.header("Content-Length");
    REQUIRE(content_length1.has_value());
    REQUIRE(content_length1.value() == "5");
    REQUIRE(req1.body == "Hello");

    auto result2 = parser.feed_data(message2.data(), message2.size());
    REQUIRE(result2.has_value());
    REQUIRE(result2->has_value());
    const auto& req2 = result2->value();
    REQUIRE(req2.method == "POST");
    REQUIRE(req2.uri.to_string() == "/submit");
    auto content_length2 = req2.header("Content-Length");
    REQUIRE(content_length2.has_value());
    REQUIRE(content_length2.value() == "13");
    REQUIRE(req2.body == "Hello, World!");
}

TEST_CASE("URI with encoded reserved characters") {
    httc::RequestParser parser{ MAX_HEADER_SIZE, MAX_BODY_SIZE };

    // /search?q=hello%26world&key=val%3Due
    // Should be parsed as:
    // path: /search
    // query: q = hello&world
    // query: key = val=ue
    std::string_view message = "GET /search?q=hello%26world&key=val%3Due HTTP/1.1\r\n"
                               "Host: example.com\r\n"
                               "\r\n";

    auto result = parser.feed_data(message.data(), message.size());
    REQUIRE(result.has_value());
    REQUIRE(result->has_value());
    const auto& req = result->value();

    // Check that we got the expected query parameters
    auto q = req.uri.query_param("q");
    REQUIRE(q.has_value());
    // This assertion will fail if the parser decodes before splitting
    // If it decodes first, it sees "q=hello&world", splitting at '&'
    CHECK(q.value() == "hello&world");

    auto key = req.uri.query_param("key");
    REQUIRE(key.has_value());
    CHECK(key.value() == "val=ue");

    // Ensure we didn't accidentally split 'world' into its own parameter
    // (which would happen if %26 was decoded to & early)
    auto world = req.uri.query_param("world");
    CHECK(!world.has_value());
}

TEST_CASE("Many small headers exceeding limit") {
    httc::RequestParser parser{ 1024, MAX_BODY_SIZE };

    std::string request_start = "GET / HTTP/1.1\r\n";
    parser.feed_data(request_start.data(), request_start.size());

    for (int i = 0; i < 200; ++i) {
        std::string header = std::format("H{}: v\r\n", i);
        auto result = parser.feed_data(header.data(), header.size());

        if (result.has_value() && !result->has_value()
            && result->error() == httc::RequestParserError::HEADER_TOO_LARGE) {
            SUCCEED("Correctly detected header overflow");
            return;
        }
    }

    std::string request_end = "\r\n";
    auto result = parser.feed_data(request_end.data(), request_end.size());

    FAIL("Parser accepted headers exceeding the configured maximum size");
}
