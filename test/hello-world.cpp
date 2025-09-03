#include <print>
#include <uvw.hpp>
#include "httc/server.h"

void handle_req(httc::Request req) {
    std::println("Got request: {}", req);
}

int main() {
    auto loop = uvw::loop::get_default();
    auto server = httc::Server(loop);

    server.set_request_handler(handle_req);

    std::println("Listening on port 1234");
    server.bind_and_listen("0.0.0.0", 1234);
}
