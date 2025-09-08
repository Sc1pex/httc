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
}
