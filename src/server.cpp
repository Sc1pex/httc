#include "httc/server.h"
#include <asio.hpp>
#include "httc/request_parser.h"
#include "httc/response.h"
#include "httc/status.h"

namespace httc {

using asio::awaitable;
using asio::use_awaitable;
using asio::ip::tcp;

awaitable<void> handle_conn(tcp::socket socket, sp<Router> router) {
    // auto req_parser = std::make_shared<RequestParser>();
    RequestParser req_parser;

    req_parser.set_on_request_complete([router, &socket](Request& req) {
        auto res = router->handle(req).value_or(Response::from_status(StatusCode::NOT_FOUND));
        res.write(socket);
    });
    req_parser.set_on_error([&socket](RequestParserError err) {
        auto res = Response::from_status(parse_error_to_status_code(err));
        res.write(socket);
        socket.close();
    });

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
        req_parser.feed_data(buf.data(), n);
    }
}

void bind_and_listen(
    const std::string& addr, unsigned int port, sp<Router> router, asio::io_context io_ctx
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
    io_ctx.run();
}

}
