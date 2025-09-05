#include <print>
#include <uvw.hpp>
#include "httc/response.h"
#include "httc/server.h"
#include "httc/status.h"

httc::Response handle_req(httc::Request req) {
    std::println("Got request: {}\n", req);

    httc::Response resp;
    resp.set_status(httc::StatusCode::OK);

    return resp;
}

int main() {
    auto loop = uvw::loop::get_default();
    auto server = httc::Server(loop);

    server.set_request_handler(handle_req);

    unsigned int port = 1234;
    std::println("Listening on port {}", port);
    server.bind_and_listen("0.0.0.0", port);
}
