#include "httc/response.hpp"
#include <asio.hpp>
#include <asio/awaitable.hpp>
#include <asio/error_code.hpp>
#include <format>
#include "httc/status.hpp"

namespace httc {

using asio::awaitable;

void Response::generate_head() {
    m_head_buffer.clear();
    auto r = status.reason();
    if (r) {
        std::format_to(std::back_inserter(m_head_buffer), "HTTP/1.1 {} {}\r\n", status.code, *r);
    } else {
        std::format_to(std::back_inserter(m_head_buffer), "HTTP/1.1 {}\r\n", status.code);
    }

    for (const auto& [key, value] : headers) {
        m_head_buffer.append(key);
        m_head_buffer.append(": ");
        m_head_buffer.append(value);
        m_head_buffer.append("\r\n");
    }
    for (const auto& cookie : cookies) {
        m_head_buffer.append("Set-Cookie: ");
        m_head_buffer.append(cookie);
        m_head_buffer.append("\r\n");
    }
    m_head_buffer.append("\r\n");
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

asio::awaitable<Response::ChunkedStream> Response::send_chunked() {
    if (m_state != State::Uninitialized) {
        throw std::runtime_error("Cannot send chunked response. Already initialized");
    }

    headers.set("Transfer-Encoding", "chunked");
    headers.unset("Content-Length");

    m_state = State::StreamChunk;

    generate_head();
    co_await m_writer.write({ asio::buffer(m_head_buffer) });

    co_return ChunkedStream{ *this };
}

asio::awaitable<Response::FixedStream> Response::send_fixed(std::size_t content_size) {
    if (m_state != State::Uninitialized) {
        throw std::runtime_error("Cannot send chunked response. Already initialized");
    }

    headers.set("Content-Length", std::to_string(content_size));

    m_state = State::StreamFixed;

    generate_head();
    co_await m_writer.write({ asio::buffer(m_head_buffer) });

    co_return FixedStream{ *this };
}

asio::awaitable<void> Response::ChunkedStream::write(std::string_view chunk) {
    if (chunk.size() == 0) {
        // Do not write empty chunks because that indicates the end of the stream
        co_return;
    }

    auto chunk_size = std::format("{:X}\r\n", chunk.size());

    co_return co_await m_parent.m_writer.write(
        {
            asio::buffer(chunk_size),
            asio::buffer(chunk),
            asio::buffer("\r\n", 2),
        }
    );
}

asio::awaitable<void> Response::ChunkedStream::end() {
    m_parent.m_state = State::Sent;
    co_return co_await m_parent.m_writer.write({ asio::buffer("0\r\n\r\n", 5) });
}

asio::awaitable<void> Response::FixedStream::write(std::string_view data) {
    co_return co_await m_parent.m_writer.write({ asio::buffer(data) });
}

awaitable<void> Response::send() {
    switch (m_state) {
    case State::Uninitialized:
        headers.set_view("Content-Length", "0");
        break;

    case State::StreamChunk:
        // Send the last close
        co_return co_await m_writer.write({ asio::buffer("0\r\n\r\n", 5) });

    case State::StreamFixed:
        co_return;

    case State::Body:
        break;

    case State::Sent:
        co_return;
    }

    generate_head();
    std::vector<asio::const_buffer> buffers;
    buffers.push_back(asio::buffer(m_head_buffer));
    if (!m_head && !m_body.empty()) {
        buffers.push_back(asio::buffer(m_body));
    }
    co_return co_await m_writer.write(buffers);
}

void Response::set_body(std::string_view body) {
    if (m_state != State::Uninitialized) {
        throw std::runtime_error("Cannot set body after response has been initialized");
    }

    headers.set("Content-Length", std::to_string(body.size()));
    m_state = State::Body;
    m_body = body;
}

void Response::add_cookie(std::string cookie) {
    cookies.push_back(std::move(cookie));
}

}
