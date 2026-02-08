#pragma once

#include <chrono>

namespace httc {
struct ServerConfig {
    std::size_t max_header_size = 16 * 1024;
    std::size_t max_body_size = 16 * 1024 * 1024;
    std::chrono::seconds request_timeout_seconds = std::chrono::seconds(30);

    constexpr ServerConfig() = default;
};
}
