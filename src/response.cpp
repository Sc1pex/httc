#include "httc/response.h"
#include <asio.hpp>
#include <asio/awaitable.hpp>
#include <asio/error_code.hpp>
#include <stdexcept>
#include "httc/status.h"

namespace httc {

using asio::awaitable;
using asio::ip::tcp;

template<typename... Args>
void write_fmt(tcp::socket& client, std::format_string<Args...> fmt, Args&&... args) {
    auto s = std::format(fmt, std::forward<Args>(args)...);
    asio::write(client, asio::buffer(s));
}

template<typename... Args>
awaitable<void>
    async_write_fmt(tcp::socket& client, std::format_string<Args...> fmt, Args&&... args) {
    auto s = std::format(fmt, std::forward<Args>(args)...);
    co_await asio::async_write(client, asio::buffer(s), asio::use_awaitable);
}

Response::Response(tcp::socket& sock, bool is_head_response) : m_sock(sock) {
    m_head = is_head_response;
    status = StatusCode::OK;
    headers.set("Content-Length", "0");
    m_state = State::Uninitialized;
}

Response Response::from_status(tcp::socket& sock, StatusCode status) {
    Response r(sock);
    r.status = status;
    return r;
}

void Response::begin_stream() {
    if (m_state != State::Uninitialized) {
        throw std::runtime_error("Response already initialized");
    }

    headers.set("Transfer-Encoding", "chunked");
    headers.unset("Content-Length");

    m_state = State::Stream;

    // Write first line and header
    write_fmt(m_sock, "HTTP/1.1 {}\r\n", status.code);
    for (const auto& [key, value] : headers) {
        write_fmt(m_sock, "{}: {}\r\n", key, value);
    }
    write_fmt(m_sock, "\r\n");
}

void Response::stream_chunk(std::string_view chunk) {
    if (m_state != State::Stream) {
        throw std::runtime_error("Response not in streaming state");
    }

    if (chunk.empty()) {
        // Do not write empty chunks because that indicates the end of the stream
        return;
    }

    write_fmt(m_sock, "{:X}\r\n", chunk.size());
    write_fmt(m_sock, "{}\r\n", chunk);
}

void Response::end_stream() {
    if (m_state != State::Stream) {
        throw std::runtime_error("Response not in streaming state");
    }

    write_fmt(m_sock, "0\r\n\r\n");
}

awaitable<void> Response::async_begin_stream() {
    if (m_state != State::Uninitialized) {
        throw std::runtime_error("Response already initialized");
    }

    headers.set("Transfer-Encoding", "chunked");
    headers.unset("Content-Length");

    m_state = State::Stream;

    // Write first line and header
    co_await async_write_fmt(m_sock, "HTTP/1.1 {}\r\n", status.code);
    for (const auto& [key, value] : headers) {
        co_await async_write_fmt(m_sock, "{}: {}\r\n", key, value);
    }
    co_await async_write_fmt(m_sock, "\r\n");
}

awaitable<void> Response::async_stream_chunk(std::string_view chunk) {
    if (m_state != State::Stream) {
        throw std::runtime_error("Response not in streaming state");
    }

    if (chunk.empty()) {
        // Do not write empty chunks because that indicates the end of the stream
        co_return;
    }

    co_await async_write_fmt(m_sock, "{:X}\r\n", chunk.size());
    co_await async_write_fmt(m_sock, "{}\r\n", chunk);
}

awaitable<void> Response::async_end_stream() {
    if (m_state != State::Stream) {
        throw std::runtime_error("Response not in streaming state");
    }

    co_await async_write_fmt(m_sock, "0\r\n\r\n");
}

awaitable<void> Response::send(Response&& res) {
    if (res.m_state == State::Stream) {
        // A streaming response is finished by the end_stream method
        co_return;
    }

    co_await async_write_fmt(res.m_sock, "HTTP/1.1 {}\r\n", res.status.code);
    for (const auto& [key, value] : res.headers) {
        co_await async_write_fmt(res.m_sock, "{}: {}\r\n", key, value);
    }
    co_await async_write_fmt(res.m_sock, "\r\n");
    if (!res.m_body.empty()) {
        co_await async_write_fmt(res.m_sock, "{}", res.m_body);
    }
}

void Response::send_blocking(Response&& res) {
    if (res.m_state == State::Stream) {
        // A streaming response is finished by the end_stream method
        return;
    }

    write_fmt(res.m_sock, "HTTP/1.1 {}\r\n", res.status.code);
    for (const auto& [key, value] : res.headers) {
        write_fmt(res.m_sock, "{}: {}\r\n", key, value);
    }
    write_fmt(res.m_sock, "\r\n");
    if (!res.m_body.empty()) {
        write_fmt(res.m_sock, "{}", res.m_body);
    }
}

void Response::set_body(std::string_view body) {
    if (m_state != State::Uninitialized) {
        throw std::runtime_error("Cannot set body after response has been initialized");
    }

    if (m_head) {
        return;
    }
    headers.set("Content-Length", std::to_string(body.size()));
    m_body = body;
}

}
