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

    SECTION("Path with parameters") {
        auto uri = httc::URI::parse("/api/v1/users/:userId");
        REQUIRE(uri.has_value());
        REQUIRE(uri->paths().size() == 4);
        REQUIRE(uri->paths()[0] == "api");
        REQUIRE(uri->paths()[1] == "v1");
        REQUIRE(uri->paths()[2] == "users");
        REQUIRE(uri->paths()[3] == ":userId");
        REQUIRE(uri->query().empty());
    }

    SECTION("Path with wildcard") {
        auto uri = httc::URI::parse("/files/*");
        REQUIRE(uri.has_value());
        REQUIRE(uri->paths().size() == 2);
        REQUIRE(uri->paths()[0] == "files");
        REQUIRE(uri->paths()[1] == "*");
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

TEST_CASE("URI matching") {
    auto uri1 = httc::URI::parse("/api/v1/users");
    auto uri2 = httc::URI::parse("/api/v1/users/123");
    auto uri3 = httc::URI::parse("/api/v1/users/:userId");
    auto uri4 = httc::URI::parse("/api/v1/*");
    auto uri5 = httc::URI::parse("/api/v1/users");

    REQUIRE(uri1.has_value());
    REQUIRE(uri2.has_value());
    REQUIRE(uri3.has_value());
    REQUIRE(uri4.has_value());
    REQUIRE(uri5.has_value());

    SECTION("Full match") {
        REQUIRE(uri1->match(*uri5) == httc::URIMatch::FULL_MATCH);
    }

    SECTION("Parameter match") {
        REQUIRE(uri3->match(*uri2) == httc::URIMatch::PARAM_MATCH);
    }

    SECTION("Wildcard match") {
        REQUIRE(uri4->match(*uri2) == httc::URIMatch::WILD_MATCH);
        REQUIRE(uri4->match(*uri1) == httc::URIMatch::WILD_MATCH);
    }

    SECTION("No match") {
        REQUIRE(uri1->match(*uri2) == httc::URIMatch::NO_MATCH);
        REQUIRE(uri2->match(*uri1) == httc::URIMatch::NO_MATCH);
        REQUIRE(uri3->match(*uri1) == httc::URIMatch::NO_MATCH);
    }

    SECTION("Match both ways") {
        auto uris = { uri1, uri2, uri3, uri4, uri5 };
        for (const auto& u1 : uris) {
            for (const auto& u2 : uris) {
                REQUIRE(u1->match(*u2) == u2->match(*u1));
            }
        }
    }

    SECTION("Different path lengths") {
        auto short_uri = httc::URI::parse("/api/v1");
        auto long_uri = httc::URI::parse("/api/v1/users/123/details");

        REQUIRE(short_uri.has_value());
        REQUIRE(long_uri.has_value());

        REQUIRE(short_uri->match(*long_uri) == httc::URIMatch::NO_MATCH);
        REQUIRE(long_uri->match(*short_uri) == httc::URIMatch::NO_MATCH);
    }

    SECTION("Parameters full match 1") {
        auto uri1 = httc::URI::parse("/api/:version/users");
        auto uri2 = httc::URI::parse("/api/:ver/users");

        REQUIRE(uri1.has_value());
        REQUIRE(uri2.has_value());

        REQUIRE(uri1->match(*uri2) == httc::URIMatch::FULL_MATCH);
    }

    SECTION("Parameters full match 2") {
        auto uri1 = httc::URI::parse("/api/users/:id");
        auto uri2 = httc::URI::parse("/api/:user/123");

        REQUIRE(uri1.has_value());
        REQUIRE(uri2.has_value());

        REQUIRE(uri1->match(*uri2) == httc::URIMatch::FULL_MATCH);
    }
}

TEST_CASE("String formatting") {
    auto uri = httc::URI::parse("/api/v1/users/:userId?active=true&role=admin");

    REQUIRE(uri.has_value());

    auto formatted = std::format("{}", *uri);
    REQUIRE(formatted == "/api/v1/users/:userId?active=true&role=admin");
}
