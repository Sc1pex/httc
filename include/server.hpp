#pragma once

#include <asio.hpp>
#include <memory>
#include <string>
#include "router.hpp"
#include "server_config.hpp"

namespace httc {

void bind_and_listen(
    std::string_view addr, unsigned int port, std::shared_ptr<Router> router,
    asio::io_context& io_ctx, const ServerConfig& config = {}
);

}
