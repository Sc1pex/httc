#include <httc/router.h>
#include <httc/server.h>
#include <cstring>
#include <fstream>
#include <print>
#include <uvw.hpp>
#include "httc/status.h"

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
    router.route("/*", [args](const httc::Request& req, httc::Response& res) {
        auto path = std::string(args->file_dir) + req.uri.path();

        auto file = std::ifstream(path);
        if (!file.is_open()) {
            res.status = httc::StatusCode::NOT_FOUND;
            return;
        }

        std::string content(
            (std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()
        );
        res.status = httc::StatusCode::OK;
        res.headers.override("Content-Length", std::to_string(content.size()));
        res.headers.override("Content-Type", "text/plain");
        res.body = std::move(content);
    });

    auto loop = uvw::loop::get_default();
    httc::Server server(loop, std::move(router));
    std::println("Listening on port {}", args->port);
    server.bind_and_listen("0.0.0.0", args->port);

    return 0;
}
