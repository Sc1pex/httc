#include <httc/request_parser.h>
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <print>

TEST_CASE("Parse request line") {
    httc::RequestParser parser;

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
    httc::RequestParser parser;

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
    httc::RequestParser parser;

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
}

TEST_CASE("Parse Content-length bodies") {
    httc::RequestParser parser;

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
    httc::RequestParser parser;

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
}

TEST_CASE("Parse in multiple chunks") {
    httc::RequestParser parser;

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
    httc::RequestParser parser;

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
