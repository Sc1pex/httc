#include <httc/router.h>
#include <httc/server.h>
#include <asio.hpp>
#include <print>

int main() {
    auto router = std::make_shared<httc::Router>();
    router->route("/ping", [](const httc::Request&, httc::Response& res) {
        res.status = httc::StatusCode::OK;
        res.headers.set("Content-Type", "text/plain");
        res.set_body("pong");
    });

    asio::io_context io_ctx;
    int port = 8080;

    std::println("Listening on port {}", port);
    httc::bind_and_listen("0.0.0.0", port, router, io_ctx);

    io_ctx.run();
    return 0;
}
