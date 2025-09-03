#include "httc/server.h"
#include <memory>
#include <print>
#include "httc/request_parser.h"

namespace httc {

template<typename... Args>
void write_fmt(
    std::shared_ptr<uvw::tcp_handle> client, std::format_string<Args...> fmt, Args&&... args
) {
    auto s = std::format(fmt, std::forward<Args>(args)...);
    client->write(s.data(), s.size());
}

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

    auto handle_data = [this, client, req_parser](const uvw::data_event& ev, uvw::tcp_handle&) {
        auto req_opt = req_parser->parse_chunk(ev.data.get(), ev.length);
        if (req_opt) {
            m_req_handler(*req_opt);
        }
    };
    client->on<uvw::data_event>(handle_data);

    client->on<uvw::end_event>([](const uvw::end_event&, uvw::tcp_handle& c) {
        c.close();
    });

    client->read();
}

void Server::set_request_handler(request_handler_fn handler) {
    m_req_handler = handler;
}

}
