#include "httc/server.h"
#include "httc/request_parser.h"
#include "httc/response.h"
#include "httc/status.h"

namespace httc {

Server::Server(sp<uvw::loop> loop) : m_loop(loop) {
}

void Server::bind_and_listen(const std::string& addr, unsigned int port) {
    auto tcp = m_loop->resource<uvw::tcp_handle>();

    tcp->on<uvw::listen_event>([this](const uvw::listen_event&, uvw::tcp_handle& tcp) {
        this->handle_conn(tcp);
    });

    tcp->bind(addr, port);
    tcp->listen();
    m_loop->run();
}

void Server::handle_conn(uvw::tcp_handle& tcp) {
    auto client = m_loop->resource<uvw::tcp_handle>();
    tcp.accept(*client);

    auto req_parser = std::make_shared<RequestParser>();

    req_parser->set_on_request_complete([this, client](const Request& req) {
        auto resp = m_req_handler(req);
        resp.write(client);
    });
    req_parser->set_on_error([client](RequestParserError err) {
        auto resp = Response::from_status(StatusCode::BAD_REQUEST);
        client->close();
    });

    client->on<uvw::data_event>([req_parser](const uvw::data_event& ev, uvw::tcp_handle&) {
        req_parser->feed_data(ev.data.get(), ev.length);
    });
    client->on<uvw::end_event>([](const uvw::end_event&, uvw::tcp_handle& c) {
        c.close();
    });

    client->read();
}

void Server::set_request_handler(request_handler_fn handler) {
    m_req_handler = handler;
}

}
