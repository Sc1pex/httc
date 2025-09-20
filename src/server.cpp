#include "httc/server.h"
#include "httc/request_parser.h"
#include "httc/response.h"
#include "httc/status.h"

namespace httc {

void handle_conn(uvw::tcp_handle& tcp, sp<Router> router) {
    auto client = tcp.parent().resource<uvw::tcp_handle>();
    tcp.accept(*client);

    auto req_parser = std::make_shared<RequestParser>();

    req_parser->set_on_request_complete([router, client](Request& req) {
        auto res = router->handle(req).value_or(Response::from_status(StatusCode::NOT_FOUND));
        res.write(client);
    });
    req_parser->set_on_error([client](RequestParserError err) {
        auto resp = Response::from_status(parse_error_to_status_code(err));
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

void bind_and_listen(
    const std::string& addr, unsigned int port, sp<Router> router, sp<uvw::loop> loop
) {
    auto tcp = loop->resource<uvw::tcp_handle>();

    tcp->on<uvw::listen_event>([router](const uvw::listen_event&, uvw::tcp_handle& tcp) {
        handle_conn(tcp, router);
    });

    tcp->bind(addr, port);
    tcp->listen();
    loop->run();
}

}
