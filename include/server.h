#pragma once

#include <asio.hpp>
#include <string>
#include "common.h"
#include "router.h"
#include "server_config.h"

namespace httc {

void bind_and_listen(
    const std::string& addr, unsigned int port, sp<Router> router, asio::io_context& io_ctx,
    const ServerConfig& config = {}
);

}
