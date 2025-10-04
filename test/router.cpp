#include "httc/router.h"
#include <asio.hpp>
#include <catch2/catch_test_macros.hpp>

namespace methods = httc::methods;

TEST_CASE("Basic routing") {
    httc::Router router;
    asio::io_context io_context;

    SECTION("Single path no method") {
        int called = 0;
        router.route("/test", [&](const httc::Request&, httc::Response&) {
            called++;
        });

        httc::Request req;
        req.method = "GET";
        req.uri = *httc::URI::parse("/test");

        auto future = asio::co_spawn(io_context, router.handle(req), asio::use_future);
        io_context.run();
        auto result = future.get();
        REQUIRE(result.has_value());

        io_context.restart();
        req.method = "Other method";
        future = asio::co_spawn(io_context, router.handle(req), asio::use_future);
        io_context.run();
        result = future.get();
        REQUIRE(result.has_value());

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

        auto future = asio::co_spawn(io_context, router.handle(req), asio::use_future);
        io_context.run();
        auto result = future.get();
        REQUIRE(result.has_value());

        io_context.restart();
        req.method = "POST";
        future = asio::co_spawn(io_context, router.handle(req), asio::use_future);
        io_context.run();
        result = future.get();
        REQUIRE(result.has_value());

        io_context.restart();
        req.method = "Other method";
        future = asio::co_spawn(io_context, router.handle(req), asio::use_future);
        io_context.run();
        result = future.get();
        REQUIRE(result.has_value());
        REQUIRE(result->status.code == 405); // Method Not Allowed

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

        auto future = asio::co_spawn(io_context, router.handle(req), asio::use_future);
        io_context.run();
        auto result = future.get();
        REQUIRE(result.has_value());
        REQUIRE(called_method == 1);
        REQUIRE(called_global == 0);

        io_context.restart();
        req.method = "POST";
        future = asio::co_spawn(io_context, router.handle(req), asio::use_future);
        io_context.run();
        result = future.get();
        REQUIRE(result.has_value());
        REQUIRE(called_method == 2);
        REQUIRE(called_global == 0);

        io_context.restart();
        req.method = "Other method";
        future = asio::co_spawn(io_context, router.handle(req), asio::use_future);
        io_context.run();
        result = future.get();
        REQUIRE(result.has_value());
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
    asio::io_context io_context;

    SECTION("No path") {
        router.route("/test", [](const httc::Request&, httc::Response&) {
        });

        httc::Request req;
        req.method = "GET";
        req.uri = *httc::URI::parse("/nope");

        auto future = asio::co_spawn(io_context, router.handle(req), asio::use_future);
        io_context.run();
        auto result = future.get();
        REQUIRE(!result.has_value());
    }

    SECTION("Path but no method") {
        router.route("/test", methods::post([](const httc::Request&, httc::Response&) {
                     }));

        httc::Request req;
        req.method = "GET";
        req.uri = *httc::URI::parse("/test");

        auto future = asio::co_spawn(io_context, router.handle(req), asio::use_future);
        io_context.run();
        auto result = future.get();
        REQUIRE(result.has_value());
        REQUIRE(result->status.code == 405); // Method Not Allowed
    }
}

TEST_CASE("Complex routing 1") {
    httc::Router router;
    asio::io_context io_context;
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

    SECTION("Exact match") {
        req.method = "GET";
        req.uri = *httc::URI::parse("/abc/def");

        auto future = asio::co_spawn(io_context, router.handle(req), asio::use_future);
        io_context.run();
        auto result = future.get();
        REQUIRE(result.has_value());
        verify_called(0);
    }

    SECTION("Param match") {
        req.method = "GET";
        req.uri = *httc::URI::parse("/abc/value");

        auto future = asio::co_spawn(io_context, router.handle(req), asio::use_future);
        io_context.run();
        auto result = future.get();
        REQUIRE(result.has_value());
        verify_called(1);
    }

    SECTION("Wildcard match 1") {
        req.method = "GET";
        req.uri = *httc::URI::parse("/abc/abc/abc");

        auto future = asio::co_spawn(io_context, router.handle(req), asio::use_future);
        io_context.run();
        auto result = future.get();
        REQUIRE(result.has_value());
        verify_called(3);
    }

    SECTION("Wildcard match 2") {
        req.method = "GET";
        req.uri = *httc::URI::parse("/abc/abc/very/deep/path");

        auto future = asio::co_spawn(io_context, router.handle(req), asio::use_future);
        io_context.run();
        auto result = future.get();
        REQUIRE(result.has_value());
        verify_called(3);
    }
}

TEST_CASE("Complex routing 2") {
    httc::Router router;
    asio::io_context io_context;
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
    SECTION("Exact match with method") {
        req.method = "GET";
        req.uri = *httc::URI::parse("/a/b");

        auto future = asio::co_spawn(io_context, router.handle(req), asio::use_future);
        io_context.run();
        auto result = future.get();
        REQUIRE(result.has_value());
        verify_called(0);
    }

    SECTION("Param match with method") {
        req.method = "POST";
        req.uri = *httc::URI::parse("/a/value");

        auto future = asio::co_spawn(io_context, router.handle(req), asio::use_future);
        io_context.run();
        auto result = future.get();
        REQUIRE(result.has_value());
        verify_called(1);
    }

    SECTION("Wildcard match no method") {
        req.method = "GET";
        req.uri = *httc::URI::parse("/a/anything/here");

        auto future = asio::co_spawn(io_context, router.handle(req), asio::use_future);
        io_context.run();
        auto result = future.get();
        REQUIRE(result.has_value());
        verify_called(2);
    }
}

TEST_CASE("Param and wildcard extraction") {
    httc::Router router;
    asio::io_context io_context;

    router.route("/files/:fileId/*", [](const httc::Request& req, httc::Response&) {
        REQUIRE(req.path_params.contains("fileId"));
        REQUIRE(req.path_params.at("fileId") == "12345");
        REQUIRE(req.wildcard_path == "path/to/file.txt");
    });

    httc::Request req;
    req.method = "GET";
    req.uri = *httc::URI::parse("/files/12345/path/to/file.txt");

    auto future = asio::co_spawn(io_context, router.handle(req), asio::use_future);
    io_context.run();
    auto result = future.get();
    REQUIRE(result.has_value());
}

TEST_CASE("Middleware") {
    httc::Router router;
    asio::io_context io_context;
    std::vector<int> call_order;

    router
        .wrap([&](const httc::Request& req, httc::Response& res, auto next) {
            call_order.push_back(1);
            next(req, res);
            call_order.push_back(5);
        })
        .wrap([&](const httc::Request& req, httc::Response& res, auto next) {
            call_order.push_back(2);
            next(req, res);
            call_order.push_back(4);
        })
        .route("/test", [&](const httc::Request&, httc::Response&) {
            call_order.push_back(3);
        });

    httc::Request req;
    req.method = "GET";
    req.uri = *httc::URI::parse("/test");

    auto future = asio::co_spawn(io_context, router.handle(req), asio::use_future);
    io_context.run();
    auto result = future.get();
    REQUIRE(result.has_value());

    REQUIRE(call_order.size() == 5);
    for (int i = 0; i < 5; i++) {
        CHECK(call_order[i] == i + 1);
    }
}
