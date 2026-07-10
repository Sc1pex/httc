#pragma once

#include <asio/awaitable.hpp>
#include <asio/buffer.hpp>
#include <asio/ip/tcp.hpp>
#include <expected>
#include <functional>
#include <string_view>
#include <vector>
#include "httc/server_config.hpp"

namespace httc {

enum class ReaderError : uint8_t {
    TIMEOUT,
    CLOSED,
    UNKNOWN,
};

template<typename T>
concept Reader = requires(T t) {
    { t.pull() } -> std::same_as<asio::awaitable<std::expected<std::string_view, ReaderError>>>;
};

template<typename T>
concept Writer = requires(T t, std::vector<asio::const_buffer> b) {
    { t.write(b) } -> std::same_as<asio::awaitable<void>>;
};

class SocketReader {
public:
    SocketReader(asio::ip::tcp::socket& socket);
    asio::awaitable<std::expected<std::string_view, ReaderError>> pull();

private:
    std::array<char, 8192> m_buffer{};
    std::reference_wrapper<asio::ip::tcp::socket> m_sock;
};

class SocketWriter {
public:
    SocketWriter(asio::ip::tcp::socket& socket);
    asio::awaitable<void> write(std::vector<asio::const_buffer> buffers);

private:
    std::reference_wrapper<asio::ip::tcp::socket> m_sock;
};

}
