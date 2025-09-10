#include "httc/uri.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Parse valid URIs") {
    SECTION("Simple path") {
        auto uri = httc::URI::parse("/index.html");
        REQUIRE(uri.has_value());
        REQUIRE(uri->paths().size() == 1);
        REQUIRE(uri->paths()[0] == "index.html");
        REQUIRE(uri->query().empty());
    }

    SECTION("Root path") {
        auto uri = httc::URI::parse("/");
        REQUIRE(uri.has_value());
        REQUIRE(uri->paths().empty());
        REQUIRE(uri->query().empty());
    }

    SECTION("Multiple path segments") {
        auto uri = httc::URI::parse("/api/v1/users");
        REQUIRE(uri.has_value());
        REQUIRE(uri->paths().size() == 3);
        REQUIRE(uri->paths()[0] == "api");
        REQUIRE(uri->paths()[1] == "v1");
        REQUIRE(uri->paths()[2] == "users");
        REQUIRE(uri->query().empty());
    }

    SECTION("Path with trailing slash") {
        auto uri = httc::URI::parse("/api/v1/users/");
        REQUIRE(uri.has_value());
        REQUIRE(uri->paths().size() == 3);
        REQUIRE(uri->paths()[0] == "api");
        REQUIRE(uri->paths()[1] == "v1");
        REQUIRE(uri->paths()[2] == "users");
        REQUIRE(uri->query().empty());
    }
}

TEST_CASE("Parse URIs with query parameters") {
    SECTION("Single query parameter") {
        auto uri = httc::URI::parse("/search?q=test");
        REQUIRE(uri.has_value());
        REQUIRE(uri->paths().size() == 1);
        REQUIRE(uri->paths()[0] == "search");
        REQUIRE(uri->query().size() == 1);
        REQUIRE(uri->query()[0].first == "q");
        REQUIRE(uri->query()[0].second == "test");
    }

    SECTION("Multiple query parameters") {
        auto uri = httc::URI::parse("/search?q=test&page=1&limit=10");
        REQUIRE(uri.has_value());
        REQUIRE(uri->paths().size() == 1);
        REQUIRE(uri->paths()[0] == "search");
        REQUIRE(uri->query().size() == 3);
        REQUIRE(uri->query()[0].first == "q");
        REQUIRE(uri->query()[0].second == "test");
        REQUIRE(uri->query()[1].first == "page");
        REQUIRE(uri->query()[1].second == "1");
        REQUIRE(uri->query()[2].first == "limit");
        REQUIRE(uri->query()[2].second == "10");
    }

    SECTION("Query parameter with empty value") {
        auto uri = httc::URI::parse("/search?q=&page=1");
        REQUIRE(uri.has_value());
        REQUIRE(uri->paths().size() == 1);
        REQUIRE(uri->paths()[0] == "search");
        REQUIRE(uri->query().size() == 2);
        REQUIRE(uri->query()[0].first == "q");
        REQUIRE(uri->query()[0].second == "");
        REQUIRE(uri->query()[1].first == "page");
        REQUIRE(uri->query()[1].second == "1");
    }

    SECTION("Query parameter without value") {
        auto uri = httc::URI::parse("/search?debug&verbose");
        REQUIRE(uri.has_value());
        REQUIRE(uri->paths().size() == 1);
        REQUIRE(uri->paths()[0] == "search");
        REQUIRE(uri->query().size() == 2);
        REQUIRE(uri->query()[0].first == "debug");
        REQUIRE(uri->query()[0].second == "");
        REQUIRE(uri->query()[1].first == "verbose");
        REQUIRE(uri->query()[1].second == "");
    }

    SECTION("Empty query string") {
        auto uri = httc::URI::parse("/search?");
        REQUIRE(uri.has_value());
        REQUIRE(uri->paths().size() == 1);
        REQUIRE(uri->paths()[0] == "search");
        REQUIRE(uri->query().empty());
    }
}

TEST_CASE("Parse invalid URIs") {
    SECTION("Missing leading slash") {
        auto uri = httc::URI::parse("invalid/path");
        REQUIRE(!uri.has_value());
    }

    SECTION("Only query string") {
        auto uri = httc::URI::parse("?q=test");
        REQUIRE(!uri.has_value());
    }
}
