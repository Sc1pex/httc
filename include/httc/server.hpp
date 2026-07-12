#pragma once

#include <asio.hpp>
#include <memory>
#include "httc/router.hpp"
#include "httc/server_config.hpp"

namespace httc {

void bind_and_listen(
    std::string_view addr, uint16_t port, std::shared_ptr<Router> router, asio::io_context& io_ctx,
    ServerConfig config = {}
);

}
