#pragma once

#include <asio/awaitable.hpp>
#include <asio/ip/tcp.hpp>
#include <expected>
#include <string_view>
#include "server_config.h"

namespace httc {

enum class ReaderError {
    TIMEOUT,
    CLOSED,
    UNKNOWN,
};

template<typename T>
concept Reader = requires(T t, std::size_t n) {
    { t.pull() } -> std::same_as<asio::awaitable<std::expected<std::string_view, ReaderError>>>;
};

class SocketReader {
public:
    SocketReader(asio::ip::tcp::socket& socket, const ServerConfig& cfg);

    asio::awaitable<std::expected<std::string_view, ReaderError>> pull();

private:
    std::array<char, 8192> m_buffer;

    asio::ip::tcp::socket& m_sock;
    const ServerConfig& m_cfg;
};

}
