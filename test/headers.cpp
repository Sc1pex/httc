#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <httc/headers.hpp>
#include <vector>

TEST_CASE("Headers case-insensitive lookup") {
    httc::Headers headers;
    headers.add("Content-Type", "application/json");
    headers.add("content-length", "123");
    headers.add("X-Custom-Header", "value1");

    SECTION("Get existing headers with different casing") {
        auto ct = headers.get_one("content-type");
        REQUIRE(ct.has_value());
        REQUIRE(ct.value() == "application/json");

        auto cl = headers.get_one("CONTENT-LENGTH");
        REQUIRE(cl.has_value());
        REQUIRE(cl.value() == "123");

        auto xch = headers.get_one("X-CUSTOM-HEADER");
        REQUIRE(xch.has_value());
        REQUIRE(xch.value() == "value1");
    }

    SECTION("Get non-existing header") {
        auto non_exist = headers.get_one("Non-Exist");
        REQUIRE(!non_exist.has_value());
    }
}

TEST_CASE("Multiple headers with same name") {
    httc::Headers headers;
    headers.add("Via", "1.1 vegur");
    headers.add("via", "1.1 varnish");

    auto range = headers.get("Via");

    std::vector<std::string> values;
    for (auto it = range.first; it != range.second; ++it) {
        values.push_back(std::string(it->second));
    }

    REQUIRE(values.size() == 2);
    REQUIRE(std::find(values.begin(), values.end(), "1.1 vegur") != values.end());
    REQUIRE(std::find(values.begin(), values.end(), "1.1 varnish") != values.end());
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
