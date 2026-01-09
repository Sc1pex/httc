#include "response.hpp"
#include <asio.hpp>
#include <asio/awaitable.hpp>
#include <asio/error_code.hpp>
#include <format>
#include "status.hpp"

namespace httc {

using asio::awaitable;

void Response::generate_status_line() {
    m_status_line = std::format("HTTP/1.1 {}\r\n", status.code);
}

std::vector<asio::const_buffer> Response::response_head() {
    std::vector<asio::const_buffer> buffers;

    const static auto header_separator = asio::buffer(": ", 2);
    const static auto crlf_buf = asio::buffer("\r\n", 2);

    generate_status_line();
    buffers.push_back(asio::buffer(m_status_line));
    for (const auto& [key, value] : headers) {
        buffers.push_back(asio::buffer(key));
        buffers.push_back(header_separator);
        buffers.push_back(asio::buffer(value));
        buffers.push_back(crlf_buf);
    }
    for (const auto& cookie : m_cookies) {
        buffers.push_back(asio::buffer("Set-Cookie"));
        buffers.push_back(header_separator);
        buffers.push_back(asio::buffer(cookie));
    }
    buffers.push_back(crlf_buf);

    return buffers;
}

Response::Response(Writer& writer, bool is_head_response) : m_writer(writer) {
    m_head = is_head_response;
    status = StatusCode::OK;
    headers.set("Content-Length", "0");
    m_state = State::Uninitialized;
}

Response Response::from_status(Writer& writer, StatusCode status) {
    Response r(writer);
    r.status = status;
    return r;
}

awaitable<void> Response::begin_stream() {
    if (m_state != State::Uninitialized) {
        throw std::runtime_error("Response already initialized");
    }

    headers.set("Transfer-Encoding", "chunked");
    headers.unset("Content-Length");

    m_state = State::Stream;

    auto buffers = response_head();
    co_return co_await m_writer.write(buffers);
}

awaitable<void> Response::stream_chunk(std::string_view chunk) {
    if (m_state != State::Stream) {
        throw std::runtime_error("Response not in streaming state");
    }

    if (chunk.empty()) {
        // Do not write empty chunks because that indicates the end of the stream
        co_return;
    }

    static const asio::const_buffer crlf_buf = asio::buffer("\r\n", 2);
    auto chunk_size = std::format("{:X}\r\n", chunk.size());
    co_return co_await m_writer.write(
        {
            asio::buffer(chunk_size),
            asio::buffer(chunk),
            crlf_buf,
        }
    );
}

awaitable<void> Response::end_stream() {
    if (m_state != State::Stream) {
        throw std::runtime_error("Response not in streaming state");
    }

    static const asio::const_buffer final_chunk_buf = asio::buffer("0\r\n\r\n", 5);
    co_return co_await m_writer.write({ final_chunk_buf });
}

awaitable<void> Response::send(Response&& res) {
    if (res.m_state == State::Stream) {
        // A streaming response is finished by the end_stream method
        co_return;
    }

    if (res.m_body.empty()) {
        res.headers.set_view("Content-Length", "0");
    }

    auto buffers = res.response_head();
    if (!res.m_head) {
        buffers.push_back(asio::buffer(res.m_body));
    }
    co_return co_await res.m_writer.write(buffers);
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

void Response::add_cookie(std::string cookie) {
    m_cookies.push_back(std::move(cookie));
}

}
