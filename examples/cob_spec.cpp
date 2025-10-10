#include <httc/router.h>
#include <httc/server.h>
#include <asio.hpp>
#include <asio/io_context.hpp>
#include <cstring>
#include <print>
#include "httc/utils/file_server.h"

struct CliArgs {
    int port;
    const char* file_dir;

    CliArgs(int argc, char** argv) {
        for (int i = 1; i < argc; ++i) {
            if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
                port = atoi(argv[++i]);
            } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
                file_dir = argv[++i];
            }
        }
    }
};

int main(int argc, char** argv) {
    CliArgs args(argc, argv);

    auto router = std::make_shared<httc::Router>();
    router->route("/*", httc::utils::FileServer(args.file_dir, true));

    asio::io_context io_ctx;

    std::println("Listening on port {}", args.port);
    httc::bind_and_listen("0.0.0.0", args.port, router, io_ctx);

    io_ctx.run();

    return 0;
}
