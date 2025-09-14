#include <httc/router.h>
#include <httc/server.h>
#include <cstring>
#include <fstream>
#include <print>
#include <uvw.hpp>
#include "httc/status.h"
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
    auto args = std::make_shared<CliArgs>(argc, argv);

    httc::Router router;
    router.route("/*", httc::utils::FileServer(args->file_dir));

    auto loop = uvw::loop::get_default();
    httc::Server server(loop, std::move(router));
    std::println("Listening on port {}", args->port);
    server.bind_and_listen("0.0.0.0", args->port);

    return 0;
}
