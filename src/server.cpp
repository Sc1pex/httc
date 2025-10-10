#include "httc/server.h"
#include <asio.hpp>
#include "httc/request_parser.h"
#include "httc/response.h"

namespace httc {

using asio::awaitable;
using asio::use_awaitable;
using asio::ip::tcp;

awaitable<void> handle_conn(tcp::socket socket, sp<Router> router) {
    RequestParser req_parser;

    auto buf = std::array<char, 1024>();

    for (;;) {
        asio::error_code ec;
        std::size_t n = co_await socket.async_read_some(
            asio::buffer(buf), asio::redirect_error(use_awaitable, ec)
        );

        // If eof is reached, parse the remaining data
        if (ec && ec != asio::error::eof) {
            break;
        }

        auto req_opt = req_parser.feed_data(buf.data(), n);
        if (!req_opt.has_value()) {
            // Incomplete request, wait for more data
            continue;
        }

        auto req_result = *req_opt;
        // If there was a parsing error, respond with the appropriate status code
        if (!req_result.has_value()) {
            auto res =
                Response::from_status(socket, parse_error_to_status_code(req_result.error()));
            res.send();
            socket.close();
            co_return;
        }

        // Successfully parsed request, handle it
        Response res{ socket };
        auto req = req_result.value();
        co_await router->handle(req, res);
        res.send();
    }
}

void bind_and_listen(
    const std::string& addr, unsigned int port, sp<Router> router, asio::io_context& io_ctx
) {
    tcp::endpoint endpoint(asio::ip::make_address(addr), port);
    tcp::acceptor acceptor(io_ctx, endpoint);

    auto listen = [&acceptor, router]() -> awaitable<void> {
        for (;;) {
            try {
                auto socket = co_await acceptor.async_accept(use_awaitable);
                auto ex = co_await asio::this_coro::executor;
                asio::co_spawn(ex, handle_conn(std::move(socket), router), asio::detached);
            } catch (std::exception& e) {
            }
        }
    };

    asio::co_spawn(io_ctx, listen(), asio::detached);
}

}
