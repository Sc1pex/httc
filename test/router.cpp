#include "httc/router.h"
#include <catch2/catch_test_macros.hpp>

namespace methods = httc::methods;

TEST_CASE("Basic routing") {
    httc::Router router;

    SECTION("Single path no method") {
        int called = 0;
        router.route("/test", [&](const httc::Request&, httc::Response&) {
            called++;
        });

        httc::Request req;
        req.method = "GET";
        req.uri = *httc::URI::parse("/test");

        httc::Response res;

        bool handled = router.handle(req, res);
        REQUIRE(handled);

        req.method = "Other method";
        handled = router.handle(req, res);
        REQUIRE(handled);

        REQUIRE(called == 2);
    }

    SECTION("Single path with method") {
        int called = 0;
        router.route(
            "/test", httc::MethodWrapper<"GET", "POST">([&](const httc::Request&, httc::Response&) {
                called++;
            })
        );

        httc::Request req;
        req.method = "GET";
        req.uri = *httc::URI::parse("/test");

        httc::Response res;

        bool handled = router.handle(req, res);
        REQUIRE(handled);

        req.method = "POST";
        handled = router.handle(req, res);
        REQUIRE(handled);

        req.method = "Other method";
        handled = router.handle(req, res);
        REQUIRE(!handled);

        REQUIRE(called == 2);
    }

    SECTION("Single path with method and global") {
        int called_global = 0;
        int called_method = 0;
        router.route("/test", [&](const httc::Request&, httc::Response&) {
            called_global++;
        });
        router.route(
            "/test", httc::MethodWrapper<"GET", "POST">([&](const httc::Request&, httc::Response&) {
                called_method++;
            })
        );

        httc::Request req;
        req.method = "GET";
        req.uri = *httc::URI::parse("/test");

        httc::Response res;

        bool handled = router.handle(req, res);
        REQUIRE(handled);
        REQUIRE(called_method == 1);
        REQUIRE(called_global == 0);

        req.method = "POST";
        handled = router.handle(req, res);
        REQUIRE(handled);
        REQUIRE(called_method == 2);
        REQUIRE(called_global == 0);

        req.method = "Other method";
        handled = router.handle(req, res);
        REQUIRE(handled);
        REQUIRE(called_method == 2);
        REQUIRE(called_global == 1);
    }
}

TEST_CASE("Routing collisions") {
    httc::Router router;

    router.route("/test", [](const httc::Request&, httc::Response&) {
    });

    REQUIRE_THROWS_AS(
        router.route(
            "/test",
            [](const httc::Request&, httc::Response&) {
            }
        ),
        httc::URICollision
    );

    router.route("/method_test", methods::get([](const httc::Request&, httc::Response&) {
                 }));

    REQUIRE_THROWS_AS(
        router.route(
            "/method_test",
            httc::MethodWrapper<"GET", "POST">([](const httc::Request&, httc::Response&) {
            })
        ),
        httc::URICollision
    );

    // Different method should be fine
    REQUIRE_NOTHROW(
        router.route("/method_test", methods::post([](const httc::Request&, httc::Response&) {
                     }))
    );
}

TEST_CASE("Invalid URIs") {
    httc::Router router;

    REQUIRE_THROWS_AS(
        router.route(
            "invalid uri",
            [](const httc::Request&, httc::Response&) {
            }
        ),
        httc::InvalidURI
    );

    REQUIRE_THROWS_AS(
        router.route(
            "/also/invalid?query=param",
            [](const httc::Request&, httc::Response&) {
            }
        ),
        httc::InvalidURI
    );
}

TEST_CASE("No matching route") {
    httc::Router router;

    SECTION("No path") {
        router.route("/test", [](const httc::Request&, httc::Response&) {
        });

        httc::Request req;
        req.method = "GET";
        req.uri = *httc::URI::parse("/nope");

        httc::Response res;

        bool handled = router.handle(req, res);
        REQUIRE(!handled);
    }

    SECTION("Path but no method") {
        router.route("/test", methods::post([](const httc::Request&, httc::Response&) {
                     }));

        httc::Request req;
        req.method = "GET";
        req.uri = *httc::URI::parse("/test");

        httc::Response res;

        bool handled = router.handle(req, res);
        REQUIRE(!handled);
    }
}

TEST_CASE("Complex routing 1") {
    httc::Router router;
    std::unordered_map<int, int> call_count;

    router.route("/abc/def", [&](const httc::Request&, httc::Response&) {
        call_count[0]++;
    });
    router.route("/abc/:param", [&](const httc::Request&, httc::Response&) {
        call_count[1]++;
    });
    router.route("/abc/abc/def", [&](const httc::Request&, httc::Response&) {
        call_count[2]++;
    });
    router.route("/abc/abc/*", [&](const httc::Request&, httc::Response&) {
        call_count[3]++;
    });

    auto verify_called = [&](int index) {
        REQUIRE(call_count.contains(index));
        REQUIRE(call_count[index] == 1);

        for (const auto& [key, value] : call_count) {
            if (key != index) {
                REQUIRE(value == 0);
            }
        }
    };

    httc::Request req;
    httc::Response res;

    SECTION("Exact match") {
        req.method = "GET";
        req.uri = *httc::URI::parse("/abc/def");

        bool handled = router.handle(req, res);
        REQUIRE(handled);
        verify_called(0);
    }

    SECTION("Param match") {
        req.method = "GET";
        req.uri = *httc::URI::parse("/abc/value");

        bool handled = router.handle(req, res);
        REQUIRE(handled);
        verify_called(1);
    }

    SECTION("Wildcard match 1") {
        req.method = "GET";
        req.uri = *httc::URI::parse("/abc/abc/abc");

        bool handled = router.handle(req, res);
        REQUIRE(handled);
        verify_called(3);
    }

    SECTION("Wildcard match 2") {
        req.method = "GET";
        req.uri = *httc::URI::parse("/abc/abc/very/deep/path");

        bool handled = router.handle(req, res);
        REQUIRE(handled);
        verify_called(3);
    }
}

TEST_CASE("Complex routing 2") {
    httc::Router router;
    std::unordered_map<int, int> call_count;

    router.route("/a/b", methods::get([&](const httc::Request&, httc::Response&) {
                     call_count[0]++;
                 }));
    router.route("/a/:param", methods::post([&](const httc::Request&, httc::Response&) {
                     call_count[1]++;
                 }));
    router.route("/a/*", ([&](const httc::Request&, httc::Response&) {
                     call_count[2]++;
                 }));

    auto verify_called = [&](int index) {
        REQUIRE(call_count.contains(index));
        REQUIRE(call_count[index] == 1);

        for (const auto& [key, value] : call_count) {
            if (key != index) {
                REQUIRE(value == 0);
            }
        }
    };

    httc::Request req;
    httc::Response res;
    SECTION("Exact match with method") {
        req.method = "GET";
        req.uri = *httc::URI::parse("/a/b");

        bool handled = router.handle(req, res);
        REQUIRE(handled);
        verify_called(0);
    }

    SECTION("Param match with method") {
        req.method = "POST";
        req.uri = *httc::URI::parse("/a/value");

        bool handled = router.handle(req, res);
        REQUIRE(handled);
        verify_called(1);
    }

    SECTION("Wildcard match no method") {
        req.method = "GET";
        req.uri = *httc::URI::parse("/a/anything/here");

        bool handled = router.handle(req, res);
        REQUIRE(handled);
        verify_called(2);
    }
}
