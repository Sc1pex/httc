#include "reader.h"

namespace httc {

SocketReader::SocketReader(asio::ip::tcp::socket& socket, const ServerConfig& cfg)
: m_sock(socket), m_cfg(cfg) {
}

asio::awaitable<std::expected<std::string_view, ReaderError>> SocketReader::pull() {
    asio::steady_timer timer(co_await asio::this_coro::executor);
    bool timeout_occurred = false;

    timer.expires_after(m_cfg.request_timeout_seconds);
    timer.async_wait([&](const asio::error_code& ec) {
        if (!ec) {
            timeout_occurred = true;
            m_sock.cancel();
        }
    });

    std::size_t n = 0;
    asio::error_code ec;
    n = co_await m_sock.async_read_some(
        asio::buffer(m_buffer), asio::redirect_error(asio::use_awaitable, ec)
    );

    timer.cancel();

    if (timeout_occurred) {
        co_return std::unexpected(ReaderError::TIMEOUT);
    }

    if (ec == asio::error::eof) {
        co_return std::unexpected(ReaderError::CLOSED);
    } else if (ec) {
        co_return std::unexpected(ReaderError::UNKNOWN);
    }

    co_return std::string_view(m_buffer.data(), n);
}

}
