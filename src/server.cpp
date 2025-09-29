#include "httc/server.h"
#include <asio.hpp>
#include <memory>
#include "httc/request_parser.h"
#include "httc/response.h"
#include "httc/status.h"

namespace httc {

using asio::ip::tcp;

void handle_conn(sp<tcp::socket> socket, sp<Router> router) {
    auto req_parser = std::make_shared<RequestParser>();

    req_parser->set_on_request_complete([router, socket](Request& req) {
        auto res = router->handle(req).value_or(Response::from_status(StatusCode::NOT_FOUND));
        res.write(socket);
    });
    req_parser->set_on_error([socket](RequestParserError err) {
        auto resp = Response::from_status(parse_error_to_status_code(err));
        socket->close();
    });

    char* read_buf = (char*) malloc(128);
    auto start_read = [socket, req_parser, read_buf](this const auto& self) -> void {
        socket->async_read_some(
            asio::buffer(read_buf, 128),
            [self, read_buf,
             req_parser](const asio::error_code& ec, std::size_t bytes_transferred) {
                if (ec && ec != asio::error::eof) {
                    return;
                }

                req_parser->feed_data(read_buf, bytes_transferred);
                if (ec != asio::error::eof) {
                    self();
                } else {
                    free(read_buf);
                }
            }
        );
    };

    start_read();
}

void bind_and_listen(
    const std::string& addr, unsigned int port, sp<Router> router, asio::io_context io_ctx
) {
    tcp::endpoint endpoint(asio::ip::make_address(addr), port);
    tcp::acceptor acceptor(io_ctx, endpoint);

    auto accept_conn = [&](this const auto& self) -> void {
        auto sock = std::make_shared<tcp::socket>(io_ctx);
        acceptor.async_accept(*sock, [&, sock](const asio::error_code& ec) {
            if (!ec) {
                handle_conn(sock, router);
            }
            self();
        });
    };

    accept_conn();
    io_ctx.run();
}

}
