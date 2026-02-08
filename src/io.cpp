#include "httc/io.hpp"
#include <asio.hpp>

namespace httc {

SocketReader::SocketReader(asio::ip::tcp::socket& socket, const ServerConfig& cfg)
: m_sock(socket), m_cfg(cfg) {
}

asio::awaitable<std::expected<std::string_view, ReaderError>> SocketReader::pull() {
    std::size_t n = 0;
    asio::error_code ec;
    n = co_await m_sock.async_read_some(
        asio::buffer(m_buffer), asio::redirect_error(asio::use_awaitable, ec)
    );

    if (ec == asio::error::eof) {
        co_return std::unexpected(ReaderError::CLOSED);
    } else if (ec == asio::error::operation_aborted) {
        co_return std::unexpected(ReaderError::TIMEOUT);
    } else if (ec) {
        co_return std::unexpected(ReaderError::UNKNOWN);
    }

    co_return std::string_view(m_buffer.data(), n);
}

SocketWriter::SocketWriter(asio::ip::tcp::socket& socket) : m_sock(socket) {
}

asio::awaitable<void> SocketWriter::write(std::vector<asio::const_buffer> buffers) {
    co_await asio::async_write(m_sock, buffers, asio::use_awaitable);
}
}
