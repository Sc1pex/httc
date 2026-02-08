#include "httc/server.hpp"
#include <asio.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <print>
#include "httc/io.hpp"
#include "httc/request_parser.hpp"
#include "httc/response.hpp"

namespace httc {

using asio::awaitable;
using asio::use_awaitable;
using asio::ip::tcp;

using namespace asio::experimental::awaitable_operators;

awaitable<void>
    handle_conn(tcp::socket socket, std::shared_ptr<Router> router, const ServerConfig& cfg) {
    SocketReader reader{ socket, cfg };
    RequestParser req_parser{ cfg.max_header_size, cfg.max_body_size, reader };

    SocketWriter writer{ socket };

    asio::steady_timer parse_request_timer(co_await asio::this_coro::executor);

    while (true) {
        parse_request_timer.expires_after(cfg.request_timeout_seconds);

        auto result =
            co_await (req_parser.next() || parse_request_timer.async_wait(asio::use_awaitable));
        std::optional<std::expected<httc::Request, httc::RequestParserError>> req_opt;
        if (result.index() == 1) {
            // Request timeout
            socket.shutdown(tcp::socket::shutdown_both);
            co_return;
        } else {
            req_opt = std::move(std::get<0>(result));
        }

        if (!req_opt.has_value()) {
            // Connection closed
            break;
        }

        auto req_result = std::move(*req_opt);
        if (!req_result.has_value()) {
            auto res =
                Response::from_status(writer, parse_error_to_status_code(req_result.error()));
            co_await res.send();
            socket.close();
            co_return;
        }

        bool success = false;
        try {
            Response res{ writer };
            auto req = std::move(req_result).value();
            co_await router->handle(req, res);
            co_await res.send();
            success = true;
        } catch (std::exception& e) {
            std::println("Error handling request: {}", e.what());
        }

        if (!success) {
            auto res = Response::from_status(writer, StatusCode::INTERNAL_SERVER_ERROR);
            co_await res.send();
            socket.close();
            co_return;
        }
    }
}

asio::awaitable<void>
    listen(tcp::acceptor acceptor, std::shared_ptr<Router> router, const ServerConfig& config) {
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
    std::string_view addr, unsigned int port, std::shared_ptr<Router> router,
    asio::io_context& io_ctx, const ServerConfig& config
) {
    tcp::endpoint endpoint(asio::ip::make_address(addr), port);
    tcp::acceptor acceptor(io_ctx, endpoint);

    asio::co_spawn(io_ctx, listen(std::move(acceptor), router, config), asio::detached);
}

}
