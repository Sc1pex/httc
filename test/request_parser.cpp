#include "httc/request_parser.h"
#include <catch2/catch_message.hpp>
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

TEST_CASE("Parse headers") {
    httc::RequestParser parser;

    SECTION("Valid headers") {
        bool callback_called = false;
        parser.set_on_request_complete([&callback_called](const httc::Request& req) {
            callback_called = true;
            REQUIRE(req.method() == "GET");
            REQUIRE(req.uri() == "/index.html");
            auto host = req.header("Host");
            REQUIRE(host.has_value());
            REQUIRE(host.value() == "example.com");
            auto user_agent = req.header("User-Agent");
            REQUIRE(user_agent.has_value());
            REQUIRE(user_agent.value() == "TestAgent/1.0");
        });
        parser.set_on_error([](httc::RequestParserError err) {
            FAIL("Error callback should not be called");
        });

        std::string_view message = "GET /index.html HTTP/1.1\r\n"
                                   "Host: example.com\r\n"
                                   "User-Agent: TestAgent/1.0\r\n"
                                   "\r\n";
        parser.feed_data(message.data(), message.size());
        REQUIRE(callback_called);
    }

    SECTION("Invalid header name") {
        bool error_called = false;
        parser.set_on_request_complete([](const httc::Request& req) {
            FAIL("Request complete callback should not be called");
        });
        parser.set_on_error([&error_called](httc::RequestParserError err) {
            error_called = true;
            REQUIRE(err == httc::RequestParserError::INVALID_HEADER);
        });

        std::string_view message = "GET /index.html HTTP/1.1\r\n"
                                   "Inva lid-Header: value\r\n"
                                   "\r\n";
        parser.feed_data(message.data(), message.size());
        REQUIRE(error_called);
    }

    SECTION("Invalid header value") {
        bool error_called = false;
        parser.set_on_request_complete([](const httc::Request& req) {
            FAIL("Request complete callback should not be called");
        });
        parser.set_on_error([&error_called](httc::RequestParserError err) {
            error_called = true;
            REQUIRE(err == httc::RequestParserError::INVALID_HEADER);
        });

        std::string_view message = "GET /index.html HTTP/1.1\r\n"
                                   "Valid-Header: value\x01\x02\x03\r\n"
                                   "\r\n";
        parser.feed_data(message.data(), message.size());
        REQUIRE(error_called);
    }

    SECTION("Header whitespace handling 1") {
        bool callback_called = false;
        parser.set_on_request_complete([&callback_called](const httc::Request& req) {
            callback_called = true;
            REQUIRE(req.method() == "GET");
            REQUIRE(req.uri() == "/index.html");
            auto header = req.header("X-Custom-Header");
            REQUIRE(header.has_value());
            REQUIRE(header.value() == "value with spaces");
        });
        parser.set_on_error([](httc::RequestParserError err) {
            FAIL("Error callback should not be called");
        });

        std::string_view message = "GET /index.html HTTP/1.1\r\n"
                                   "X-Custom-Header:    value with spaces   \r\n"
                                   "\r\n";
        parser.feed_data(message.data(), message.size());
        REQUIRE(callback_called);
    }

    SECTION("Header whitespace handling 2") {
        bool callback_called = false;
        parser.set_on_request_complete([&callback_called](const httc::Request& req) {
            callback_called = true;
            REQUIRE(req.method() == "GET");
            REQUIRE(req.uri() == "/index.html");
            auto header = req.header("X-Custom-Header");
            REQUIRE(header.has_value());
            REQUIRE(header.value() == "value with spaces");
        });
        parser.set_on_error([](httc::RequestParserError err) {
            FAIL("Error callback should not be called");
        });

        std::string_view message = "GET /index.html HTTP/1.1\r\n"
                                   "X-Custom-Header:value with spaces\r\n"
                                   "\r\n";
        parser.feed_data(message.data(), message.size());
        REQUIRE(callback_called);
    }

    SECTION("Header with whitespace 3") {
        bool error_called = false;
        parser.set_on_request_complete([](const httc::Request& req) {
            FAIL("Request complete callback should not be called");
        });
        parser.set_on_error([&error_called](httc::RequestParserError err) {
            error_called = true;
            REQUIRE(err == httc::RequestParserError::INVALID_HEADER);
        });

        std::string_view message = "GET /index.html HTTP/1.1\r\n"
                                   "X-Custom-Header : value with spaces\r\n"
                                   "\r\n";
        parser.feed_data(message.data(), message.size());
        REQUIRE(error_called);
    }

    SECTION("Missing colon in header") {
        bool error_called = false;
        parser.set_on_request_complete([](const httc::Request& req) {
            FAIL("Request complete callback should not be called");
        });
        parser.set_on_error([&error_called](httc::RequestParserError err) {
            error_called = true;
            REQUIRE(err == httc::RequestParserError::INVALID_HEADER);
        });

        std::string_view message = "GET /index.html HTTP/1.1\r\n"
                                   "Invalid-Header value\r\n"
                                   "\r\n";
        parser.feed_data(message.data(), message.size());
        REQUIRE(error_called);
    }

    SECTION("Empty header name") {
        bool error_called = false;
        parser.set_on_request_complete([](const httc::Request& req) {
            FAIL("Request complete callback should not be called");
        });
        parser.set_on_error([&error_called](httc::RequestParserError err) {
            error_called = true;
            REQUIRE(err == httc::RequestParserError::INVALID_HEADER);
        });

        std::string_view message = "GET /index.html HTTP/1.1\r\n"
                                   ": value\r\n"
                                   "\r\n";
        parser.feed_data(message.data(), message.size());
        REQUIRE(error_called);
    }

    SECTION("Empty header value") {
        bool callback_called = false;
        parser.set_on_request_complete([&callback_called](const httc::Request& req) {
            callback_called = true;
            REQUIRE(req.method() == "GET");
            REQUIRE(req.uri() == "/index.html");
            auto header = req.header("X-Empty-Header");
            REQUIRE(header.has_value());
            REQUIRE(header.value() == "");
        });
        parser.set_on_error([](httc::RequestParserError err) {
            FAIL("Error callback should not be called");
        });

        std::string_view message = "GET /index.html HTTP/1.1\r\n"
                                   "X-Empty-Header: \r\n"
                                   "\r\n";
        parser.feed_data(message.data(), message.size());
        REQUIRE(callback_called);
    }
}

TEST_CASE("Parse Content-length bodies") {
    httc::RequestParser parser;

    SECTION("Valid Content-Length body") {
        bool callback_called = false;
        parser.set_on_request_complete([&callback_called](const httc::Request& req) {
            callback_called = true;
            REQUIRE(req.method() == "POST");
            REQUIRE(req.uri() == "/submit");
            auto content_length = req.header("Content-Length");
            REQUIRE(content_length.has_value());
            REQUIRE(content_length.value() == "13");
            REQUIRE(req.body() == "Hello, World!");
        });
        parser.set_on_error([](httc::RequestParserError err) {
            FAIL("Error callback should not be called");
        });

        std::string_view message = "POST /submit HTTP/1.1\r\n"
                                   "Host: example.com\r\n"
                                   "Content-Length: 13\r\n"
                                   "\r\n"
                                   "Hello, World!";
        parser.feed_data(message.data(), message.size());
        REQUIRE(callback_called);
    }

    SECTION("Invalid Content-Length value") {
        bool error_called = false;
        parser.set_on_request_complete([](const httc::Request& req) {
            FAIL("Request complete callback should not be called");
        });
        parser.set_on_error([&error_called](httc::RequestParserError err) {
            error_called = true;
            REQUIRE(err == httc::RequestParserError::INVALID_HEADER);
        });

        std::string_view message = "POST /submit HTTP/1.1\r\n"
                                   "Host: example.com\r\n"
                                   "Content-Length: invalid\r\n"
                                   "\r\n"
                                   "Hello, World!";
        parser.feed_data(message.data(), message.size());
        REQUIRE(error_called);
    }

    SECTION("Content-Length exceeds maximum size") {
        bool error_called = false;
        parser.set_on_request_complete([](const httc::Request& req) {
            FAIL("Request complete callback should not be called");
        });
        parser.set_on_error([&error_called](httc::RequestParserError err) {
            error_called = true;
            REQUIRE(err == httc::RequestParserError::CONTENT_TOO_LARGE);
        });

        std::string_view message = "POST /submit HTTP/1.1\r\n"
                                   "Host: example.com\r\n"
                                   "Content-Length: 10485771\r\n" // 10 MB + 1 byte
                                   "\r\n";
        parser.feed_data(message.data(), message.size());
        REQUIRE(error_called);
    }
}

TEST_CASE("Parse chunked bodies") {
    httc::RequestParser parser;

    SECTION("Valid chunked body") {
        bool callback_called = false;
        parser.set_on_request_complete([&callback_called](const httc::Request& req) {
            callback_called = true;
            REQUIRE(req.method() == "POST");
            REQUIRE(req.uri() == "/submit");
            auto transfer_encoding = req.header("Transfer-Encoding");
            REQUIRE(transfer_encoding.has_value());
            REQUIRE(transfer_encoding.value() == "chunked");
            REQUIRE(req.body() == "Hello, World");
        });
        parser.set_on_error([](httc::RequestParserError err) {
            FAIL("Error callback should not be called");
        });

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
        parser.feed_data(message.data(), message.size());
        REQUIRE(callback_called);
    }

    SECTION("Invalid chunk size") {
        bool error_called = false;
        parser.set_on_request_complete([](const httc::Request& req) {
            FAIL("Request complete callback should not be called");
        });
        parser.set_on_error([&error_called](httc::RequestParserError err) {
            error_called = true;
        });

        std::string_view message = "POST /submit HTTP/1.1\r\n"
                                   "Host: example.com\r\n"
                                   "Transfer-Encoding: chunked\r\n"
                                   "\r\n"
                                   "invalid\r\n"
                                   "Hello\r\n"
                                   "0\r\n"
                                   "\r\n";
        parser.feed_data(message.data(), message.size());
        REQUIRE(error_called);
    }
}

TEST_CASE("Parse in multiple chunks") {
    httc::RequestParser parser;

    SECTION("Withoud body") {
        bool callback_called = false;
        parser.set_on_request_complete([&callback_called](const httc::Request& req) {
            callback_called = true;
            CHECK(req.method() == "GET");
            CHECK(req.uri() == "/index.html");
            auto host = req.header("Host");
            CHECK(host.has_value());
            CHECK(host.value() == "example.com");
        });
        parser.set_on_error([](httc::RequestParserError err) {
            FAIL("Error callback should not be called");
        });

        std::string_view message = "GET /index.html HTTP/1.1\r\n"
                                   "Host: example.com\r\n"
                                   "\r\n";
        for (int i = 0; i < message.size(); i++) {
            INFO("Feeding byte " << i);
            parser.feed_data(&message[i], 1);

            if (i < message.size() - 1) {
                REQUIRE(!callback_called);
            } else {
                REQUIRE(callback_called);
            }
        }
    }

    SECTION("With Content-Length body") {
        bool callback_called = false;
        parser.set_on_request_complete([&callback_called](const httc::Request& req) {
            callback_called = true;
            CHECK(req.method() == "POST");
            CHECK(req.uri() == "/submit");
            auto content_length = req.header("Content-Length");
            CHECK(content_length.has_value());
            CHECK(content_length.value() == "13");
            CHECK(req.body() == "Hello, World!");
        });
        parser.set_on_error([](httc::RequestParserError err) {
            FAIL("Error callback should not be called");
        });

        std::string_view message = "POST /submit HTTP/1.1\r\n"
                                   "Host: example.com\r\n"
                                   "Content-Length: 13\r\n"
                                   "\r\n"
                                   "Hello, World!";
        for (int i = 0; i < message.size(); i++) {
            INFO("Feeding byte " << i);
            parser.feed_data(&message[i], 1);

            if (i < message.size() - 1) {
                REQUIRE(!callback_called);
            } else {
                REQUIRE(callback_called);
            }
        }
    }

    SECTION("With chunked body") {
        bool callback_called = false;
        parser.set_on_request_complete([&callback_called](const httc::Request& req) {
            callback_called = true;
            CHECK(req.method() == "POST");
            CHECK(req.uri() == "/submit");
            auto transfer_encoding = req.header("Transfer-Encoding");
            CHECK(transfer_encoding.has_value());
            CHECK(transfer_encoding.value() == "chunked");
            CHECK(req.body() == "Hello, World");
        });
        parser.set_on_error([](httc::RequestParserError err) {
            FAIL("Error callback should not be called");
        });

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
        for (int i = 0; i < message.size(); i++) {
            INFO("Feeding byte " << i);
            parser.feed_data(&message[i], 1);

            if (i < message.size() - 1) {
                REQUIRE(!callback_called);
            } else {
                REQUIRE(callback_called);
            }
        }
    }
}

TEST_CASE("Multiple requests") {
    httc::RequestParser parser;

    int req_count = 0;
    parser.set_on_request_complete([&req_count](const httc::Request& req) {
        req_count++;
        if (req_count == 1) {
            REQUIRE(req.method() == "POST");
            REQUIRE(req.uri() == "/abc");
            auto content_length = req.header("Content-Length");
            REQUIRE(content_length.has_value());
            REQUIRE(content_length.value() == "5");
            REQUIRE(req.body() == "Hello");
        } else if (req_count == 2) {
            REQUIRE(req.method() == "POST");
            REQUIRE(req.uri() == "/submit");
            auto content_length = req.header("Content-Length");
            REQUIRE(content_length.has_value());
            REQUIRE(content_length.value() == "13");
            REQUIRE(req.body() == "Hello, World!");
        } else {
            FAIL("Unexpected request count");
        }
    });
    parser.set_on_error([](httc::RequestParserError err) {
        INFO("Error callback called with error: " << static_cast<int>(err));
        FAIL("Error callback should not be called");
    });

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
    parser.feed_data(message1.data(), message1.size());
    parser.feed_data(message2.data(), message2.size());
    REQUIRE(req_count == 2);
}
