#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <httc/io.hpp>
#include <httc/request_parser.hpp>
#include "async_test.hpp"

const std::size_t MAX_HEADER_SIZE = 16 * 1024;
const std::size_t MAX_BODY_SIZE = 10 * 1024 * 1024;

class StringReader {
public:
    asio::awaitable<std::expected<std::string_view, httc::ReaderError>> pull() {
        if (read) {
            co_return std::unexpected(httc::ReaderError::CLOSED);
        } else {
            read = true;
            co_return str;
        }
    };

    void set_data(std::string data) {
        str = data;
        read = false;
    }

private:
    bool read = false;
    std::string str;
};

class StringByteReader {
public:
    asio::awaitable<std::expected<std::string_view, httc::ReaderError>> pull() {
        if (pos >= str.size()) {
            co_return std::unexpected(httc::ReaderError::CLOSED);
        } else {
            co_return std::string_view(&str[pos++], 1);
        }
    };

    void set_data(std::string_view data) {
        str = data;
        pos = 0;
    }

    bool is_eof() const {
        return pos >= str.size();
    }

private:
    std::string_view str;
    std::size_t pos = 0;
};

class StringArrayReader {
public:
    asio::awaitable<std::expected<std::string_view, httc::ReaderError>> pull() {
        if (index >= data.size()) {
            co_return std::unexpected(httc::ReaderError::CLOSED);
        } else {
            co_return data[index++];
        }
    };
    void set_data(const std::vector<std::string>& input_data) {
        data = input_data;
        index = 0;
    }

private:
    std::vector<std::string> data;
    std::size_t index = 0;
};

ASYNC_TEST_CASE("Parse request line") {
    auto reader = StringReader{};
    httc::RequestParser parser{ MAX_HEADER_SIZE, MAX_BODY_SIZE, reader };

    SECTION("Valid request line") {
        reader.set_data("GET /index.html HTTP/1.1\r\n\r\n");
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        REQUIRE(req.method == "GET");
        REQUIRE(req.uri.to_string() == "/index.html");
    }

    SECTION("Valid request line with url encoding") {
        reader.set_data("GET /abc%20def HTTP/1.1\r\n\r\n");
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        REQUIRE(req.method == "GET");
        REQUIRE(req.uri.to_string() == "/abc def");
    }

    SECTION("Invalid HTTP version") {
        reader.set_data("GET /index.html HTTP/2.0\r\n\r\n");
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_REQUEST_LINE);
    }

    SECTION("Request line with leading CRLF") {
        reader.set_data("\r\n\r\nGET /index.html HTTP/1.1\r\n\r\n");
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        REQUIRE(req.method == "GET");
        REQUIRE(req.uri.to_string() == "/index.html");
    }

    SECTION("Invalid method token") {
        reader.set_data("GE==T /index.html HTTP/1.1\r\n\r\n");
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_REQUEST_LINE);
    }

    SECTION("Invalid request line 1") {
        reader.set_data("INVALID_REQUEST_LINE\r\n\r\n");
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_REQUEST_LINE);
    }

    SECTION("Invalid request line 2") {
        reader.set_data("GET /index.html\r\n\r\n");
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_REQUEST_LINE);
    }

    SECTION("Invalid request line 3") {
        reader.set_data("GET /index.html HTTP/1.1 EXTRA\r\n\r\n");
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_REQUEST_LINE);
    }
}

ASYNC_TEST_CASE("Parse query parameters") {
    auto reader = StringReader{};
    httc::RequestParser parser{ MAX_HEADER_SIZE, MAX_BODY_SIZE, reader };

    SECTION("Valid query parameters") {
        reader.set_data("GET /search?q=test&page=1 HTTP/1.1\r\n\r\n");
        auto result = co_await parser.next();
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
        reader.set_data("GET /search?q=&p= HTTP/1.1\r\n\r\n");
        auto result = co_await parser.next();
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

ASYNC_TEST_CASE("Parse headers") {
    StringReader reader;
    httc::RequestParser parser{ MAX_HEADER_SIZE, MAX_BODY_SIZE, reader };

    SECTION("Valid headers") {
        reader.set_data(
            "GET /index.html HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "User-Agent: TestAgent/1.0\r\n"
            "\r\n"
        );
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        REQUIRE(req.method == "GET");
        REQUIRE(req.uri.to_string() == "/index.html");
        auto host = req.headers.get_one("Host");
        REQUIRE(host.has_value());
        REQUIRE(host.value() == "example.com");
        auto user_agent = req.headers.get_one("User-Agent");
        REQUIRE(user_agent.has_value());
        REQUIRE(user_agent.value() == "TestAgent/1.0");
    }

    SECTION("Invalid header name") {
        reader.set_data(
            "GET /index.html HTTP/1.1\r\n"
            "Inva lid-Header: value\r\n"
            "\r\n"
        );
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_HEADER);

        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_HEADER);
    }

    SECTION("Invalid header value") {
        reader.set_data(
            "GET /index.html HTTP/1.1\r\n"
            "Valid-Header: value\x01\x02\x03\r\n"
            "\r\n"
        );
        auto result = co_await parser.next();

        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_HEADER);
    }

    SECTION("Header whitespace handling 1") {
        reader.set_data(
            "GET /index.html HTTP/1.1\r\n"
            "X-Custom-Header:    value with spaces   \r\n"
            "\r\n"
        );
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        REQUIRE(req.method == "GET");
        REQUIRE(req.uri.to_string() == "/index.html");
        auto header = req.headers.get_one("X-Custom-Header");
        REQUIRE(header.has_value());
        REQUIRE(header.value() == "value with spaces");
    }

    SECTION("Header whitespace handling 2") {
        reader.set_data(
            "GET /index.html HTTP/1.1\r\n"
            "X-Custom-Header:value with spaces\r\n"
            "\r\n"
        );
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        REQUIRE(req.method == "GET");
        REQUIRE(req.uri.to_string() == "/index.html");
        auto header = req.headers.get_one("X-Custom-Header");
        REQUIRE(header.has_value());
        REQUIRE(header.value() == "value with spaces");
    }

    SECTION("Header with whitespace 3") {
        reader.set_data(
            "GET /index.html HTTP/1.1\r\n"
            "X-Custom-Header : value with spaces\r\n"
            "\r\n"
        );
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_HEADER);
    }

    SECTION("Missing colon in header") {
        reader.set_data(
            "GET /index.html HTTP/1.1\r\n"
            "Invalid-Header value\r\n"
            "\r\n"
        );
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_HEADER);
    }

    SECTION("Empty header name") {
        reader.set_data(
            "GET /index.html HTTP/1.1\r\n"
            ": value\r\n"
            "\r\n"
        );
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_HEADER);
    }

    SECTION("Empty header value") {
        reader.set_data(
            "GET /index.html HTTP/1.1\r\n"
            "X-Empty-Header: \r\n"
            "\r\n"
        );
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        REQUIRE(req.method == "GET");
        REQUIRE(req.uri.to_string() == "/index.html");
        auto header = req.headers.get_one("X-Empty-Header");
        REQUIRE(header.has_value());
        REQUIRE(header.value() == "");
    }

    SECTION("Headers exceeding maximum size") {
        std::string large_header_value(MAX_HEADER_SIZE, 'a');
        reader.set_data(
            "GET /index.html HTTP/1.1\r\n"
            "X-Large-Header: "
            + large_header_value
            + "\r\n"
              "\r\n"
        );
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::HEADER_TOO_LARGE);
    }
}

ASYNC_TEST_CASE("Parse Content-length bodies") {
    auto reader = StringReader{};
    httc::RequestParser parser{ MAX_HEADER_SIZE, MAX_BODY_SIZE, reader };

    SECTION("Valid Content-Length body") {
        reader.set_data(
            "POST /submit HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Content-Length: 13\r\n"
            "\r\n"
            "Hello, World!"
        );
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        REQUIRE(req.method == "POST");
        REQUIRE(req.uri.to_string() == "/submit");
        auto content_length = req.headers.get_one("Content-Length");
        REQUIRE(content_length.has_value());
        REQUIRE(content_length.value() == "13");
        REQUIRE(req.body == "Hello, World!");
    }

    SECTION("Invalid Content-Length value") {
        reader.set_data(
            "POST /submit HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Content-Length: invalid\r\n"
            "\r\n"
            "Hello, World!"
        );
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_HEADER);
    }

    SECTION("Content-Length exceeds maximum size") {
        reader.set_data(
            "POST /submit HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Content-Length: 10485771\r\n" // 10 MB + 1 byte
            "\r\n"
        );
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::CONTENT_TOO_LARGE);
    }
}

ASYNC_TEST_CASE("Parse chunked bodies") {
    auto reader = StringReader{};
    httc::RequestParser parser{ MAX_HEADER_SIZE, MAX_BODY_SIZE, reader };

    SECTION("Valid chunked body") {
        reader.set_data(
            "POST /submit HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n"
            "5\r\n"
            "Hello\r\n"
            "7\r\n"
            ", World\r\n"
            "0\r\n"
            "\r\n"
        );
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        REQUIRE(req.method == "POST");
        REQUIRE(req.uri.to_string() == "/submit");
        auto transfer_encoding = req.headers.get_one("Transfer-Encoding");
        REQUIRE(transfer_encoding.has_value());
        REQUIRE(transfer_encoding.value() == "chunked");
        REQUIRE(req.body == "Hello, World");
    }

    SECTION("Invalid chunk size") {
        reader.set_data(
            "POST /submit HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n"
            "invalid\r\n"
            "Hello\r\n"
            "0\r\n"
            "\r\n"
        );
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(!result->has_value());
        REQUIRE(result->error() == httc::RequestParserError::INVALID_CHUNK_ENCODING);
    }

    SECTION("With trailers") {
        reader.set_data(
            "POST /submit HTTP/1.1\r\n"
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
            "\r\n"
        );

        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());

        const auto& req = result->value();
        auto trailers = req.trailers;

        auto trailer_header = trailers.get_one("Trailer-Header");
        REQUIRE(trailer_header.has_value());
        REQUIRE(trailer_header.value() == "TrailerValue");
        auto other_trailer = trailers.get_one("Other-Trailer");
        REQUIRE(other_trailer.has_value());
        REQUIRE(other_trailer.value() == "AnotherValue");
    }
}

ASYNC_TEST_CASE("Parse in multiple chunks") {
    auto reader = StringByteReader{};
    httc::RequestParser parser{ MAX_HEADER_SIZE, MAX_BODY_SIZE, reader };

    SECTION("Without body") {
        reader.set_data(
            "GET /index.html HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "\r\n"
        );
        auto result = co_await parser.next();

        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        CHECK(req.method == "GET");
        CHECK(req.uri.to_string() == "/index.html");
        auto host = req.headers.get_one("Host");
        CHECK(host.has_value());
        CHECK(host.value() == "example.com");

        REQUIRE(reader.is_eof());
    }

    SECTION("With Content-Length body") {
        reader.set_data(
            "POST /submit HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Content-Length: 13\r\n"
            "\r\n"
            "Hello, World!"
        );
        auto result = co_await parser.next();

        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        CHECK(req.method == "POST");
        CHECK(req.uri.to_string() == "/submit");
        auto content_length = req.headers.get_one("Content-Length");
        CHECK(content_length.has_value());
        CHECK(content_length.value() == "13");
        CHECK(req.body == "Hello, World!");

        REQUIRE(reader.is_eof());
    }

    SECTION("With chunked body") {
        reader.set_data(
            "POST /submit HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n"
            "5\r\n"
            "Hello\r\n"
            "7\r\n"
            ", World\r\n"
            "0\r\n"
            "\r\n"
        );
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        CHECK(req.method == "POST");
        CHECK(req.uri.to_string() == "/submit");
        auto transfer_encoding = req.headers.get_one("Transfer-Encoding");
        CHECK(transfer_encoding.has_value());
        CHECK(transfer_encoding.value() == "chunked");
        CHECK(req.body == "Hello, World");

        REQUIRE(reader.is_eof());
    }
}

ASYNC_TEST_CASE("Multiple requests") {
    StringArrayReader reader;
    httc::RequestParser parser{ MAX_HEADER_SIZE, MAX_BODY_SIZE, reader };

    reader.set_data(
        { "POST /abc HTTP/1.1\r\n"
          "Host: example.com\r\n"
          "Content-Length: 5\r\n"
          "\r\n"
          "HelloPOST",

          " /submit HTTP/1.1\r\n"
          "Host: example.com\r\n"
          "Content-Length: 13\r\n"
          "\r\n"
          "Hello, World!" }
    );

    auto result1 = co_await parser.next();
    REQUIRE(result1.has_value());
    REQUIRE(result1->has_value());
    const auto& req1 = result1->value();
    REQUIRE(req1.method == "POST");
    REQUIRE(req1.uri.to_string() == "/abc");
    auto content_length1 = req1.headers.get_one("Content-Length");
    REQUIRE(content_length1.has_value());
    REQUIRE(content_length1.value() == "5");
    REQUIRE(req1.body == "Hello");

    auto result2 = co_await parser.next();
    REQUIRE(result2.has_value());
    REQUIRE(result2->has_value());
    const auto& req2 = result2->value();
    REQUIRE(req2.method == "POST");
    REQUIRE(req2.uri.to_string() == "/submit");
    auto content_length2 = req2.headers.get_one("Content-Length");
    REQUIRE(content_length2.has_value());
    REQUIRE(content_length2.value() == "13");
    REQUIRE(req2.body == "Hello, World!");
}

ASYNC_TEST_CASE("URI with encoded reserved characters") {
    StringReader reader;
    httc::RequestParser parser{ MAX_HEADER_SIZE, MAX_BODY_SIZE, reader };

    // /search?q=hello%26world&key=val%3Due
    // Should be parsed as:
    // path: /search
    // query: q = hello&world
    // query: key = val=ue
    reader.set_data(
        "GET /search?q=hello%26world&key=val%3Due HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n"
    );

    auto result = co_await parser.next();
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

ASYNC_TEST_CASE("Many small headers exceeding limit") {
    StringArrayReader reader;
    httc::RequestParser parser{ 1024, MAX_BODY_SIZE, reader };

    auto data = std::vector<std::string>{};
    data.push_back("GET / HTTP/1.1\r\n");
    for (int i = 0; i < 200; ++i) {
        data.push_back(std::format("H{}: v\r\n", i));
    }
    data.push_back("\r\n");

    reader.set_data(data);
    auto result = co_await parser.next();

    REQUIRE(result.has_value());
    REQUIRE(!result->has_value());
    REQUIRE(result->error() == httc::RequestParserError::HEADER_TOO_LARGE);
}

ASYNC_TEST_CASE("Cookies") {
    StringReader reader;
    httc::RequestParser parser{ MAX_HEADER_SIZE, MAX_BODY_SIZE, reader };

    SECTION("Single Cookie header") {
        reader.set_data(
            "GET / HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Cookie: sessionId=abc123\r\n"
            "\r\n"
        );
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        const auto& cookies = req.cookies;
        REQUIRE(cookies.at("sessionId") == "abc123");
        REQUIRE(cookies.size() == 1);
    }

    SECTION("Multiple Cookie headers") {
        reader.set_data(
            "GET / HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Cookie: sessionId=abc123\r\n"
            "Cookie: theme=light\r\n"
            "\r\n"
        );
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        const auto& cookies = req.cookies;
        REQUIRE(cookies.size() == 2);
        REQUIRE(cookies.at("sessionId") == "abc123");
        REQUIRE(cookies.at("theme") == "light");
    }

    SECTION("Multiple cookies in one header") {
        reader.set_data(
            "GET / HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Cookie: sessionId=abc123; theme=light; lang=en\r\n"
            "\r\n"
        );
        auto result = co_await parser.next();
        REQUIRE(result.has_value());
        REQUIRE(result->has_value());
        const auto& req = result->value();
        const auto& cookies = req.cookies;
        REQUIRE(cookies.size() == 3);
        REQUIRE(cookies.at("sessionId") == "abc123");
        REQUIRE(cookies.at("theme") == "light");
        REQUIRE(cookies.at("lang") == "en");
    }
}
