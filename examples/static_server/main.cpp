#include <asio.hpp>
#include <httc/router.hpp>
#include <httc/server.hpp>
#include <httc/utils/file_handlers.hpp>
#include <print>

int main() {
    auto router = std::make_shared<httc::Router>();

    // These paths are relative to where the executable is run.
    // In our case, we'll run it from the example folder.
    router->route("/", httc::utils::FileHandler("index.html"));
    router->route("/public/*", httc::utils::DirectoryHandler("./public", true));

    asio::io_context io_ctx;
    int port = 8080;

    std::println("Static server listening on port {}", port);
    std::println("Visit: http://localhost:{}", port);

    httc::bind_and_listen("0.0.0.0", port, router, io_ctx);

    io_ctx.run();
    return 0;
}
