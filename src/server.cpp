#include "server.h"
#include <asio.hpp>
#include <print>
#include "request_parser.h"
#include "response.h"

namespace httc {

using asio::awaitable;
using asio::use_awaitable;
using asio::ip::tcp;

awaitable<std::optional<std::tuple<asio::error_code, std::size_t>>> async_read_with_timeout(
    tcp::socket& socket, asio::mutable_buffer buffer, std::chrono::seconds timeout_seconds
) {
    asio::steady_timer timer(co_await asio::this_coro::executor);
    bool timeout_occurred = false;

    timer.expires_after(timeout_seconds);
    timer.async_wait([&](const asio::error_code& ec) {
        if (!ec) {
            timeout_occurred = true;
            socket.cancel();
        }
    });

    auto [ec, n] = co_await socket.async_read_some(buffer, asio::as_tuple(use_awaitable));

    timer.cancel();

    if (timeout_occurred) {
        co_return std::nullopt;
    } else {
        co_return std::make_optional(std::make_tuple(ec, n));
    }
}

awaitable<void> handle_conn(tcp::socket socket, sp<Router> router, const ServerConfig& cfg) {
    RequestParser req_parser{ cfg.max_header_size, cfg.max_body_size };

    auto buf = std::array<char, 1024>();

    for (;;) {
        auto read_result = co_await async_read_with_timeout(
            socket, asio::buffer(buf), cfg.request_timeout_seconds
        );
        if (!read_result.has_value()) {
            std::println("Request timed out");
            socket.close();
            co_return;
        }

        auto [ec, n] = read_result.value();

        // If eof is reached, parse the remaining data
        if (ec && ec != asio::error::eof) {
            break;
        }

        auto req_opt = req_parser.feed_data(buf.data(), n);
        while (req_opt.has_value()) {
            auto req_result = *req_opt;
            if (!req_result.has_value()) {
                auto res =
                    Response::from_status(socket, parse_error_to_status_code(req_result.error()));
                co_await Response::send(std::move(res));
                socket.close();
                co_return;
            }

            try {
                Response res{ socket };
                auto req = req_result.value();
                co_await router->handle(req, res);
                co_await Response::send(std::move(res));
            } catch (std::exception& e) {
                std::println("Error handling request: {}", e.what());
                auto res = Response::from_status(socket, StatusCode::INTERNAL_SERVER_ERROR);
                Response::send_blocking(std::move(res));
                socket.close();
                co_return;
            }

            req_opt = req_parser.feed_data("", 0);
        }
    }
}

asio::awaitable<void>
    listen(tcp::acceptor acceptor, sp<Router> router, const ServerConfig& config) {
    for (;;) {
        try {
            auto socket = co_await acceptor.async_accept(use_awaitable);
            auto ex = co_await asio::this_coro::executor;
            asio::co_spawn(ex, handle_conn(std::move(socket), router, config), asio::detached);
        } catch (std::exception& e) {
            std::println("Error accepting connection: {}", e.what());
        }
    }
}

void bind_and_listen(
    const std::string& addr, unsigned int port, sp<Router> router, asio::io_context& io_ctx,
    const ServerConfig& config
) {
    tcp::endpoint endpoint(asio::ip::make_address(addr), port);
    tcp::acceptor acceptor(io_ctx, endpoint);

    asio::co_spawn(io_ctx, listen(std::move(acceptor), router, config), asio::detached);
}
}
