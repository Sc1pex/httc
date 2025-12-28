#pragma once
#include <asio.hpp>
#include <catch2/catch_test_macros.hpp>

template<typename F>
void run_async_test(F&& test_coroutine_factory) {
    asio::io_context io_ctx;
    asio::co_spawn(io_ctx, test_coroutine_factory(), [](std::exception_ptr e) {
        if (e)
            std::rethrow_exception(e);
    });
    io_ctx.run();
}

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)

#define ASYNC_TEST_CASE(name, ...) \
    static asio::awaitable<void> CONCAT(_async_test_body_, __LINE__)(); \
    TEST_CASE(name, ##__VA_ARGS__) { \
        run_async_test(CONCAT(_async_test_body_, __LINE__)); \
    } \
    asio::awaitable<void> CONCAT(_async_test_body_, __LINE__)()
