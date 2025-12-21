#pragma once

#include <asio.hpp>
#include <string>
#include "common.h"
#include "router.h"

namespace httc {

struct ServerConfig {
    std::size_t max_header_size = 16 * 1024;
    std::size_t max_body_size = 16 * 1024 * 1024;
};

void bind_and_listen(
    const std::string& addr, unsigned int port, sp<Router> router, asio::io_context& io_ctx,
    const ServerConfig& config = {}
);

}
