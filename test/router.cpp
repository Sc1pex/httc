#include <httc/request.hpp>
#include <httc/response.hpp>
#include <httc/router.hpp>
#include <httc/status.hpp>
#include <asio.hpp>
#include <catch2/catch_test_macros.hpp>

namespace methods = httc::methods;
using asio::awaitable;
using asio::ip::tcp;

httc::Response get_response(httc::Router& router, httc::Request& req) {
    asio::io_context io_context;
    tcp::socket mock_sock{ io_context.get_executor() };

    httc::Response res{ mock_sock };
    auto future = asio::co_spawn(io_context, router.handle(req, res), asio::use_future);
    io_context.run();
    future.get();

    return res;
}

TEST_CASE("Basic routing") {
    httc::Router router;

    SECTION("Single path no method") {
        int called = 0;
        router.route("/test", [&](const httc::Request&, httc::Response&) -> awaitable<void> {
            called++;
            co_return;
        });

        httc::Request req;
        req.method = "GET";
        req.uri = *httc::URI::parse("/test");

        {
            auto res = get_response(router, req);
            REQUIRE(res.status.code == 200);
        }

        req.method = "Other method";
        {
            auto res = get_response(router, req);
            REQUIRE(res.status.code == 200);
        }

        REQUIRE(called == 2);
    }

    SECTION("Single path with method") {
        int called = 0;
        router.route(
            "/test",
            httc::MethodWrapper<"GET", "POST">([&](const httc::Request&, httc::Response&) -> awaitable<void> {
                called++;
                co_return;
            })
        );

        httc::Request req;
        req.method = "GET";
        req.uri = *httc::URI::parse("/test");

        {
            auto res = get_response(router, req);
            REQUIRE(res.status.code == 200);
        }

        req.method = "POST";
        {
            auto res = get_response(router, req);
            REQUIRE(res.status.code == 200);
        }

        req.method = "Other method";
        {
            auto res = get_response(router, req);
            REQUIRE(res.status.code == 405);
        }

        REQUIRE(called == 2);
    }

    SECTION("Single path with method and global") {
        int called_global = 0;
        int called_method = 0;
        router.route("/test", [&](const httc::Request&, httc::Response&) -> awaitable<void> {
            called_global++;
            co_return;
        });
        router.route(
            "/test",
            httc::MethodWrapper<"GET", "POST">([&](const httc::Request&, httc::Response&) -> awaitable<void> {
                called_method++;
                co_return;
            })
        );

        httc::Request req;
        req.method = "GET";
        req.uri = *httc::URI::parse("/test");

        REQUIRE_NOTHROW(get_response(router, req));
        REQUIRE(called_method == 1);
        REQUIRE(called_global == 0);

        req.method = "POST";
        REQUIRE_NOTHROW(get_response(router, req));
        REQUIRE(called_method == 2);
        REQUIRE(called_global == 0);

        req.method = "Other method";
        REQUIRE_NOTHROW(get_response(router, req));
        REQUIRE(called_method == 2);
        REQUIRE(called_global == 1);
    }
}

TEST_CASE("Routing collisions") {
    httc::Router router;

    router.route("/test", [](const httc::Request&, httc::Response&) -> awaitable<void> {
        co_return;
    });

    REQUIRE_THROWS_AS(
        router.route(
            "/test",
            [](const httc::Request&, httc::Response&) -> awaitable<void> {
                co_return;
            }
        ),
        httc::URICollision
    );

    router.route("/method_test", methods::get([](const httc::Request&, httc::Response&) -> awaitable<void> {
                     co_return;
                 }));

    REQUIRE_THROWS_AS(
        router.route(
            "/method_test",
            httc::MethodWrapper<"GET", "POST">([](const httc::Request&, httc::Response&) -> awaitable<void> {
                co_return;
            })
        ),
        httc::URICollision
    );

    // Different method should be fine
    REQUIRE_NOTHROW(
        router.route("/method_test", methods::post([](const httc::Request&, httc::Response&) -> awaitable<void> {
                         co_return;
                     }))
    );
}

TEST_CASE("Invalid URIs") {
    httc::Router router;

    REQUIRE_THROWS_AS(
        router.route(
            "invalid uri",
            [](const httc::Request&, httc::Response&) -> awaitable<void> {
                co_return;
            }
        ),
        httc::InvalidURI
    );

    REQUIRE_THROWS_AS(
        router.route(
            "/also/invalid?query=param",
            [](const httc::Request&, httc::Response&) -> awaitable<void> {
                co_return;
            }
        ),
        httc::InvalidURI
    );
}

TEST_CASE("No matching route") {
    httc::Router router;
    asio::io_context io_context;

    SECTION("No path") {
        router.route("/test", [](const httc::Request&, httc::Response&) -> awaitable<void> {
            co_return;
        });

        httc::Request req;
        req.method = "GET";
        req.uri = *httc::URI::parse("/nope");

        auto res = get_response(router, req);
        REQUIRE(res.status.code == 404);
    }

    SECTION("Path but no method") {
        router.route("/test", methods::post([](const httc::Request&, httc::Response&) -> awaitable<void> {
                         co_return;
                     }));

        httc::Request req;
        req.method = "GET";
        req.uri = *httc::URI::parse("/test");

        auto res = get_response(router, req);
        REQUIRE(res.status.code == 405);
    }
}

TEST_CASE("Complex routing 1") {
    httc::Router router;
    asio::io_context io_context;
    std::unordered_map<int, int> call_count;

    router.route("/abc/def", [&](const httc::Request&, httc::Response&) -> awaitable<void> {
        call_count[0]++;
        co_return;
    });
    router.route("/abc/:param", [&](const httc::Request&, httc::Response&) -> awaitable<void> {
        call_count[1]++;
        co_return;
    });
    router.route("/abc/abc/def", [&](const httc::Request&, httc::Response&) -> awaitable<void> {
        call_count[2]++;
        co_return;
    });
    router.route("/abc/abc/*", [&](const httc::Request&, httc::Response&) -> awaitable<void> {
        call_count[3]++;
        co_return;
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

        auto res = get_response(router, req);
        REQUIRE(res.status.code == 200);
        verify_called(0);
    }

    SECTION("Param match") {
        req.method = "GET";
        req.uri = *httc::URI::parse("/abc/value");

        auto res = get_response(router, req);
        REQUIRE(res.status.code == 200);
        verify_called(1);
    }

    SECTION("Wildcard match 1") {
        req.method = "GET";
        req.uri = *httc::URI::parse("/abc/abc/abc");

        auto res = get_response(router, req);
        REQUIRE(res.status.code == 200);
        verify_called(3);
    }

    SECTION("Wildcard match 2") {
        req.method = "GET";
        req.uri = *httc::URI::parse("/abc/abc/very/deep/path");

        auto res = get_response(router, req);
        REQUIRE(res.status.code == 200);
        verify_called(3);
    }
}

TEST_CASE("Complex routing 2") {
    httc::Router router;
    asio::io_context io_context;
    std::unordered_map<int, int> call_count;

    router.route("/a/b", methods::get([&](const httc::Request&, httc::Response&) -> awaitable<void> {
                     call_count[0]++;
                     co_return;
                 }));
    router.route("/a/:param", methods::post([&](const httc::Request&, httc::Response&) -> awaitable<void> {
                     call_count[1]++;
                     co_return;
                 }));
    router.route("/a/*", ([&](const httc::Request&, httc::Response&) -> awaitable<void> {
                     call_count[2]++;
                     co_return;
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

        auto res = get_response(router, req);
        REQUIRE(res.status.code == 200);
        verify_called(0);
    }

    SECTION("Param match with method") {
        req.method = "POST";
        req.uri = *httc::URI::parse("/a/value");

        auto res = get_response(router, req);
        REQUIRE(res.status.code == 200);
        verify_called(1);
    }

    SECTION("Wildcard match no method") {
        req.method = "GET";
        req.uri = *httc::URI::parse("/a/anything/here");

        auto res = get_response(router, req);
        REQUIRE(res.status.code == 200);
        verify_called(2);
    }
}

TEST_CASE("Param and wildcard extraction") {
    httc::Router router;
    asio::io_context io_context;

    router.route("/files/:fileId/*", [](const httc::Request& req, httc::Response&) -> awaitable<void> {
        REQUIRE(req.path_params.contains("fileId"));
        REQUIRE(req.path_params.at("fileId") == "12345");
        REQUIRE(req.wildcard_path == "path/to/file.txt");
        co_return;
    });

    httc::Request req;
    req.method = "GET";
    req.uri = *httc::URI::parse("/files/12345/path/to/file.txt");

    auto res = get_response(router, req);
    REQUIRE(res.status.code == 200);
}

TEST_CASE("Middleware") {
    httc::Router router;
    asio::io_context io_context;
    std::vector<int> call_order;

    router
        .wrap([&](const httc::Request& req, httc::Response& res, auto next) -> awaitable<void> {
            call_order.push_back(1);
            co_await next(req, res);
            call_order.push_back(5);
        })
        .wrap([&](const httc::Request& req, httc::Response& res, auto next) -> awaitable<void> {
            call_order.push_back(2);
            co_await next(req, res);
            call_order.push_back(4);
        })
        .route("/test", httc::MethodWrapper<"GET">([&](const httc::Request&, httc::Response&) -> awaitable<void> {
                   call_order.push_back(3);
                   co_return;
               }));

    SECTION("GET request") {
        httc::Request req;
        req.method = "GET";
        req.uri = *httc::URI::parse("/test");

        auto res = get_response(router, req);
        REQUIRE(res.status.code == 200);

        REQUIRE(call_order.size() == 5);
        for (int i = 0; i < 5; i++) {
            CHECK(call_order[i] == i + 1);
        }
    }

    SECTION("OPTIONS request") {
        httc::Request req;
        req.method = "OPTIONS";
        req.uri = *httc::URI::parse("/test");
        call_order.clear();

        auto res = get_response(router, req);
        REQUIRE(res.status.code == 200);
        REQUIRE(call_order.size() == 4);
        REQUIRE(call_order[0] == 1);
        REQUIRE(call_order[1] == 2);
        REQUIRE(call_order[2] == 4);
        REQUIRE(call_order[3] == 5);
    }
}