#pragma once

#include <asio/awaitable.hpp>
#include <asio/buffer.hpp>
#include <asio/ip/tcp.hpp>
#include <expected>
#include <string_view>
#include <vector>
#include "server_config.hpp"

namespace httc {

enum class ReaderError {
    TIMEOUT,
    CLOSED,
    UNKNOWN,
};

template<typename T>
concept Reader = requires(T t) {
    { t.pull() } -> std::same_as<asio::awaitable<std::expected<std::string_view, ReaderError>>>;
};

struct Writer {
    virtual asio::awaitable<void> write(std::vector<asio::const_buffer> buffers) = 0;
    virtual ~Writer() = default;
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

class SocketWriter : public Writer {
public:
    SocketWriter(asio::ip::tcp::socket& socket);
    asio::awaitable<void> write(std::vector<asio::const_buffer> buffers) override;

private:
    asio::ip::tcp::socket& m_sock;
};

}
