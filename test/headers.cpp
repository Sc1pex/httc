#include "httc/headers.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Headers case-insensitive lookup") {
    httc::Headers headers;
    headers.add("Content-Type", "application/json");
    headers.add("content-length", "123");
    headers.add("X-Custom-Header", "value1");

    SECTION("Get existing headers with different casing") {
        auto ct = headers.get("content-type");
        REQUIRE(ct.has_value());
        REQUIRE(ct.value() == "application/json");

        auto cl = headers.get("CONTENT-LENGTH");
        REQUIRE(cl.has_value());
        REQUIRE(cl.value() == "123");

        auto xch = headers.get("X-CUSTOM-HEADER");
        REQUIRE(xch.has_value());
        REQUIRE(xch.value() == "value1");
    }

    SECTION("Get non-existing header") {
        auto non_exist = headers.get("Non-Exist");
        REQUIRE(!non_exist.has_value());
    }
}

TEST_CASE("Combine multiple headers with same name") {
    httc::Headers headers;
    headers.add("Set-Cookie", "id=123");
    headers.add("set-cookie", "session=abc");
    headers.add("SET-COOKIE", "theme=dark");

    auto sc = headers.get("Set-Cookie");
    REQUIRE(sc.has_value());
    REQUIRE(sc.value() == "id=123, session=abc, theme=dark");
}

TEST_CASE("Headers iterator interface") {
    httc::Headers headers;
    headers.add("Header-One", "value1");
    headers.add("Header-Two", "value2");

    std::unordered_map<std::string, std::string> expected = { { "Header-One", "value1" },
                                                              { "Header-Two", "value2" } };

    auto it = std::find_if(headers.begin(), headers.end(), [](const auto& pair) {
        return pair.first == "Header-One";
    });
    REQUIRE(it != headers.end());
    REQUIRE(it->second == "value1");
}
