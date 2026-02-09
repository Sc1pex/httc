#include <catch2/catch_test_macros.hpp>
#include <httc/response.hpp>
#include <httc/status.hpp>
#include <string>
#include <vector>
#include "async_test.hpp"

using namespace httc;

struct MockWriter {
    std::string output;
    std::vector<std::string> writes;

    asio::awaitable<void> write(std::vector<asio::const_buffer> buffers) {
        std::string current_write;
        for (const auto& buf : buffers) {
            std::string_view part(static_cast<const char*>(buf.data()), buf.size());
            output += part;
            current_write += part;
        }
        writes.push_back(current_write);
        co_return;
    }
};

ASYNC_TEST_CASE("Response - Buffered Body") {
    MockWriter writer;
    Response res(writer);

    SECTION("Simple Body") {
        res.status = StatusCode::OK;
        res.set_body("Hello World");

        co_await res.send();

        REQUIRE(writer.writes.size() == 1);
        REQUIRE(writer.output.find("HTTP/1.1 200 OK\r\n") != std::string::npos);
        REQUIRE(writer.output.find("Content-Length: 11\r\n") != std::string::npos);
        REQUIRE(writer.output.find("\r\n\r\nHello World") != std::string::npos);
    }

    SECTION("Empty Body") {
        res.status = StatusCode::NO_CONTENT;
        co_await res.send();

        REQUIRE(writer.writes.size() == 1);
        REQUIRE(writer.output.find("HTTP/1.1 204 No Content\r\n") != std::string::npos);
        REQUIRE(writer.output.find("Content-Length: 0\r\n") != std::string::npos);
    }
}

ASYNC_TEST_CASE("Response - Headers and Cookies") {
    MockWriter writer;
    Response res(writer);

    res.headers.set("Content-Type", "application/json");
    res.add_cookie("session=123");
    res.add_cookie("theme=dark");
    res.set_body("{}");

    co_await res.send();

    REQUIRE(writer.output.find("Content-Type: application/json\r\n") != std::string::npos);
    REQUIRE(writer.output.find("Set-Cookie: session=123\r\n") != std::string::npos);
    REQUIRE(writer.output.find("Set-Cookie: theme=dark\r\n") != std::string::npos);
}

ASYNC_TEST_CASE("Response - Chunked Streaming") {
    MockWriter writer;
    Response res(writer);

    SECTION("Manual End") {
        auto stream = co_await res.send_chunked();

        REQUIRE(writer.writes.size() == 1);
        REQUIRE(writer.output.find("Transfer-Encoding: chunked\r\n") != std::string::npos);

        co_await stream.write("Wiki");
        co_await stream.write("pedia");
        co_await stream.end();

        REQUIRE(writer.output.find("4\r\nWiki\r\n") != std::string::npos);
        REQUIRE(writer.output.find("5\r\npedia\r\n") != std::string::npos);
        REQUIRE(writer.output.find("0\r\n\r\n") != std::string::npos);
    }
}

ASYNC_TEST_CASE("Response - Fixed Stream") {
    MockWriter writer;
    Response res(writer);

    SECTION("Streaming exact length") {
        std::string data = "Hello World";
        auto stream = co_await res.send_fixed(data.size());

        REQUIRE(writer.writes.size() == 1);
        REQUIRE(writer.output.find("Content-Length: 11\r\n") != std::string::npos);

        co_await stream.write("Hello");
        co_await stream.write(" ");
        co_await stream.write("World");

        REQUIRE(writer.output.find("Hello World") != std::string::npos);
        REQUIRE(writer.output.find("5\r\nHello\r\n") == std::string::npos);
    }
}

ASYNC_TEST_CASE("Response - Cookies with various body types") {
    MockWriter writer;
    Response res(writer);

    res.add_cookie("session=123");

    SECTION("No Body") {
        res.status = StatusCode::NO_CONTENT;
        co_await res.send();

        REQUIRE(writer.output.find("Set-Cookie: session=123\r\n") != std::string::npos);
        REQUIRE(writer.output.find("Content-Length: 0\r\n") != std::string::npos);
    }

    SECTION("Buffered Body") {
        res.set_body("Buffered Data");
        co_await res.send();

        REQUIRE(writer.output.find("Set-Cookie: session=123\r\n") != std::string::npos);
        REQUIRE(writer.output.find("Content-Length: 13\r\n") != std::string::npos);
        REQUIRE(writer.output.find("\r\n\r\nBuffered Data") != std::string::npos);
    }

    SECTION("Chunked Stream") {
        auto stream = co_await res.send_chunked();

        REQUIRE(writer.output.find("Set-Cookie: session=123\r\n") != std::string::npos);
        REQUIRE(writer.output.find("Transfer-Encoding: chunked\r\n") != std::string::npos);

        co_await stream.write("Chunk");
        co_await stream.end();
    }

    SECTION("Fixed Stream") {
        auto stream = co_await res.send_fixed(5);

        REQUIRE(writer.output.find("Set-Cookie: session=123\r\n") != std::string::npos);
        REQUIRE(writer.output.find("Content-Length: 5\r\n") != std::string::npos);

        co_await stream.write("Fixed");
    }
}
