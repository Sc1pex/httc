#include "httc/request_parser.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Parse request line") {
    httc::RequestParser parser;

    SECTION("Valid request line") {
        bool callback_called = false;
        parser.set_on_request_complete([&callback_called](const httc::Request& req) {
            callback_called = true;
            REQUIRE(req.method() == "GET");
            REQUIRE(req.uri() == "/index.html");
        });
        parser.set_on_error([](httc::RequestParserError err) {
            FAIL("Error callback should not be called");
        });

        std::string_view message = "GET /index.html HTTP/1.1\r\n\r\n";
        parser.feed_data(message.data(), message.size());
        REQUIRE(callback_called);
    }

    SECTION("Invalid HTTP version") {
        bool error_called = false;
        parser.set_on_request_complete([](const httc::Request& req) {
            FAIL("Request complete callback should not be called");
        });
        parser.set_on_error([&error_called](httc::RequestParserError err) {
            error_called = true;
            REQUIRE(err == httc::RequestParserError::INVALID_REQUEST_LINE);
        });

        std::string_view message = "GET /index.html HTTP/2.0\r\n\r\n";
        parser.feed_data(message.data(), message.size());
        REQUIRE(error_called);
    }

    SECTION("Request line with leading CRLF") {
        bool callback_called = false;
        parser.set_on_request_complete([&callback_called](const httc::Request& req) {
            callback_called = true;
            REQUIRE(req.method() == "GET");
            REQUIRE(req.uri() == "/index.html");
        });
        parser.set_on_error([](httc::RequestParserError err) {
            FAIL("Error callback should not be called");
        });

        std::string_view message = "\r\n\r\nGET /index.html HTTP/1.1\r\n\r\n";
        parser.feed_data(message.data(), message.size());
        REQUIRE(callback_called);
    }

    SECTION("Invalid method token") {
        bool error_called = false;
        parser.set_on_request_complete([](const httc::Request& req) {
            FAIL("Request complete callback should not be called");
        });
        parser.set_on_error([&error_called](httc::RequestParserError err) {
            error_called = true;
            REQUIRE(err == httc::RequestParserError::INVALID_REQUEST_LINE);
        });

        std::string_view message = "GE==T /index.html HTTP/1.1\r\n\r\n";
        parser.feed_data(message.data(), message.size());
        REQUIRE(error_called);
    }

    SECTION("Invalid request line 1") {
        bool error_called = false;
        parser.set_on_request_complete([](const httc::Request& req) {
            FAIL("Request complete callback should not be called");
        });
        parser.set_on_error([&error_called](httc::RequestParserError err) {
            error_called = true;
            REQUIRE(err == httc::RequestParserError::INVALID_REQUEST_LINE);
        });

        std::string_view message = "INVALID_REQUEST_LINE\r\n\r\n";
        parser.feed_data(message.data(), message.size());
        REQUIRE(error_called);
    }

    SECTION("Invalid request line 2") {
        bool error_called = false;
        parser.set_on_request_complete([](const httc::Request& req) {
            FAIL("Request complete callback should not be called");
        });
        parser.set_on_error([&error_called](httc::RequestParserError err) {
            error_called = true;
            REQUIRE(err == httc::RequestParserError::INVALID_REQUEST_LINE);
        });

        std::string_view message = "GET /index.html\r\n\r\n";
        parser.feed_data(message.data(), message.size());
        REQUIRE(error_called);
    }

    SECTION("Invalid request line 3") {
        bool error_called = false;
        parser.set_on_request_complete([](const httc::Request& req) {
            FAIL("Request complete callback should not be called");
        });
        parser.set_on_error([&error_called](httc::RequestParserError err) {
            error_called = true;
            REQUIRE(err == httc::RequestParserError::INVALID_REQUEST_LINE);
        });

        std::string_view message = "GET /index.html HTTP/1.1 EXTRA\r\n\r\n";
        parser.feed_data(message.data(), message.size());
        REQUIRE(error_called);
    }
}
