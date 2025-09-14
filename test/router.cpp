#include "httc/router.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Router basic routing") {
    httc::Router router;
    bool handler_called = false;
    std::string captured_path;

    SECTION("Add route with method") {
        router.route("GET", "/test", [&](const httc::Request& req, httc::Response& res) {
            handler_called = true;
            captured_path = std::format("{}", req.uri);
            REQUIRE(req.handler_path == "/test");
        });

        httc::Request req;
        req.method = "GET";
        auto uri = httc::URI::parse("/test");
        REQUIRE(uri.has_value());
        req.uri = *uri;

        httc::Response res;
        bool handled = router.handle(req, res);

        REQUIRE(handled);
        REQUIRE(handler_called);
        REQUIRE(captured_path == "/test");
    }

    SECTION("Add route without method") {
        router.route("/test", [&](const httc::Request& req, httc::Response& res) {
            handler_called = true;
            captured_path = std::format("{}", req.uri);
        });

        httc::Request req;
        req.method = "POST";
        auto uri = httc::URI::parse("/test");
        REQUIRE(uri.has_value());
        req.uri = *uri;

        httc::Response res;
        bool handled = router.handle(req, res);

        REQUIRE(handled);
        REQUIRE(handler_called);
        REQUIRE(captured_path == "/test");
    }

    SECTION("Method-specific routing") {
        int get_calls = 0;
        int post_calls = 0;

        router.route("GET", "/test", [&](const httc::Request& req, httc::Response& res) {
            get_calls++;
        });

        router.route("POST", "/test", [&](const httc::Request& req, httc::Response& res) {
            post_calls++;
        });

        httc::Request get_req;
        get_req.method = "GET";
        auto uri = httc::URI::parse("/test");
        REQUIRE(uri.has_value());
        get_req.uri = *uri;

        httc::Request post_req;
        post_req.method = "POST";
        post_req.uri = *uri;

        httc::Response res;

        REQUIRE(router.handle(get_req, res));
        REQUIRE(get_calls == 1);
        REQUIRE(post_calls == 0);

        REQUIRE(router.handle(post_req, res));
        REQUIRE(get_calls == 1);
        REQUIRE(post_calls == 1);
    }
}

TEST_CASE("Router path matching") {
    httc::Router router;
    std::string matched_path;

    SECTION("Exact path match") {
        router.route("GET", "/api/v1/users", [&](const httc::Request& req, httc::Response& res) {
            matched_path = std::format("{}", req.uri);
        });

        httc::Request req;
        req.method = "GET";
        auto uri = httc::URI::parse("/api/v1/users");
        REQUIRE(uri.has_value());
        req.uri = *uri;

        httc::Response res;
        bool handled = router.handle(req, res);

        REQUIRE(handled);
        REQUIRE(matched_path == "/api/v1/users");
    }

    SECTION("Parameter matching") {
        router.route("GET", "/users/:id", [&](const httc::Request& req, httc::Response& res) {
            matched_path = std::format("{}", req.uri);
        });

        httc::Request req;
        req.method = "GET";
        auto uri = httc::URI::parse("/users/123");
        REQUIRE(uri.has_value());
        req.uri = *uri;

        httc::Response res;
        bool handled = router.handle(req, res);

        REQUIRE(handled);
        REQUIRE(matched_path == "/users/123");
    }

    SECTION("Wildcard matching") {
        router.route("GET", "/files/*", [&](const httc::Request& req, httc::Response& res) {
            matched_path = std::format("{}", req.uri);
        });

        httc::Request req;
        req.method = "GET";
        auto uri = httc::URI::parse("/files/images/photo.jpg");
        REQUIRE(uri.has_value());
        req.uri = *uri;

        httc::Response res;
        bool handled = router.handle(req, res);

        REQUIRE(handled);
        REQUIRE(matched_path == "/files/images/photo.jpg");
    }

    SECTION("No match") {
        router.route("GET", "/api/users", [&](const httc::Request& req, httc::Response& res) {
            matched_path = std::format("{}", req.uri);
        });

        httc::Request req;
        req.method = "GET";
        auto uri = httc::URI::parse("/api/posts");
        REQUIRE(uri.has_value());
        req.uri = *uri;

        httc::Response res;
        bool handled = router.handle(req, res);

        REQUIRE(!handled);
        REQUIRE(matched_path.empty());
    }
}

TEST_CASE("Router priority handling") {
    httc::Router router;
    std::string handler_type;

    SECTION("Full match takes priority over parameter match") {
        router.route("GET", "/users/:id", [&](const httc::Request& req, httc::Response& res) {
            handler_type = "parameter";
        });

        router.route("GET", "/users/admin", [&](const httc::Request& req, httc::Response& res) {
            handler_type = "exact";
        });

        httc::Request req;
        req.method = "GET";
        auto uri = httc::URI::parse("/users/admin");
        REQUIRE(uri.has_value());
        req.uri = *uri;

        httc::Response res;
        bool handled = router.handle(req, res);

        REQUIRE(handled);
        REQUIRE(handler_type == "exact");
    }

    SECTION("Parameter match takes priority over wildcard match") {
        router.route("GET", "/api/*", [&](const httc::Request& req, httc::Response& res) {
            handler_type = "wildcard";
        });

        router.route("GET", "/api/:version", [&](const httc::Request& req, httc::Response& res) {
            handler_type = "parameter";
        });

        httc::Request req;
        req.method = "GET";
        auto uri = httc::URI::parse("/api/v1");
        REQUIRE(uri.has_value());
        req.uri = *uri;

        httc::Response res;
        bool handled = router.handle(req, res);

        REQUIRE(handled);
        REQUIRE(handler_type == "parameter");
    }

    SECTION("Duplicate routes throw collision error") {
        router.route("GET", "/test", [](const httc::Request& req, httc::Response& res) {
        });

        REQUIRE_THROWS_AS(
            router.route(
                "GET", "/test",
                [](const httc::Request& req, httc::Response& res) {
                }
            ),
            httc::URICollision
        );
    }
}

TEST_CASE("Router method filtering") {
    httc::Router router;
    bool handler_called = false;

    router.route("POST", "/submit", [&](const httc::Request& req, httc::Response& res) {
        handler_called = true;
    });

    SECTION("Correct method") {
        httc::Request req;
        req.method = "POST";
        auto uri = httc::URI::parse("/submit");
        REQUIRE(uri.has_value());
        req.uri = *uri;

        httc::Response res;
        bool handled = router.handle(req, res);

        REQUIRE(handled);
        REQUIRE(handler_called);
    }

    SECTION("Wrong method") {
        httc::Request req;
        req.method = "GET";
        auto uri = httc::URI::parse("/submit");
        REQUIRE(uri.has_value());
        req.uri = *uri;

        httc::Response res;
        bool handled = router.handle(req, res);

        REQUIRE(!handled);
        REQUIRE(!handler_called);
    }
}

TEST_CASE("Router with query parameters") {
    httc::Router router;
    std::string received_query;

    router.route("GET", "/search", [&](const httc::Request& req, httc::Response& res) {
        auto& query = req.uri.query();
        if (!query.empty()) {
            received_query = query[0].first + "=" + query[0].second;
        }
    });

    httc::Request req;
    req.method = "GET";
    auto uri = httc::URI::parse("/search?q=test");
    REQUIRE(uri.has_value());
    req.uri = *uri;

    httc::Response res;
    bool handled = router.handle(req, res);

    REQUIRE(handled);
    REQUIRE(received_query == "q=test");
}

TEST_CASE("Router error handling") {
    httc::Router router;

    SECTION("Invalid URI throws exception") {
        REQUIRE_THROWS_AS(
            router.route(
                "GET", "invalid-path",
                [](const httc::Request&, httc::Response&) {
                }
            ),
            httc::InvalidURI
        );
    }

    SECTION("URI collision detection") {
        router.route("GET", "/users/:id", [](const httc::Request&, httc::Response&) {
        });

        REQUIRE_THROWS_AS(
            router.route(
                "GET", "/users/:userId",
                [](const httc::Request&, httc::Response&) {
                }
            ),
            httc::URICollision
        );
    }

    SECTION("Different methods don't collide") {
        router.route("GET", "/users/:id", [](const httc::Request&, httc::Response&) {
        });

        REQUIRE_NOTHROW(
            router.route("POST", "/users/:id", [](const httc::Request&, httc::Response&) {
            })
        );
    }

    SECTION("Method-agnostic routes can collide") {
        router.route("/users/:id", [](const httc::Request&, httc::Response&) {
        });

        REQUIRE_THROWS_AS(
            router.route(
                "/users/:userId",
                [](const httc::Request&, httc::Response&) {
                }
            ),
            httc::URICollision
        );
    }
}

TEST_CASE("Router chaining") {
    httc::Router router;
    int handler_count = 0;

    SECTION("Method chaining works") {
        auto& result = router
                           .route(
                               "GET", "/test1",
                               [&](const httc::Request&, httc::Response&) {
                                   handler_count++;
                               }
                           )
                           .route(
                               "POST", "/test2",
                               [&](const httc::Request&, httc::Response&) {
                                   handler_count++;
                               }
                           )
                           .route("/test3", [&](const httc::Request&, httc::Response&) {
                               handler_count++;
                           });

        REQUIRE(&result == &router);

        // Test that all routes were added
        httc::Response res;

        httc::Request req1;
        req1.method = "GET";
        auto uri1 = httc::URI::parse("/test1");
        REQUIRE(uri1.has_value());
        req1.uri = *uri1;

        httc::Request req2;
        req2.method = "POST";
        auto uri2 = httc::URI::parse("/test2");
        REQUIRE(uri2.has_value());
        req2.uri = *uri2;

        httc::Request req3;
        req3.method = "DELETE";
        auto uri3 = httc::URI::parse("/test3");
        REQUIRE(uri3.has_value());
        req3.uri = *uri3;

        REQUIRE(router.handle(req1, res));
        REQUIRE(router.handle(req2, res));
        REQUIRE(router.handle(req3, res));
        REQUIRE(handler_count == 3);
    }
}
