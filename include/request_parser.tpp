#pragma once

#include <asio.hpp>
#include <charconv>
#include <expected>
#include <optional>
#include <string_view>
#include "common.h"
#include "headers.h"
#include "request.h"
#include "request_parser.h"

namespace httc {

template<Reader R>
RequestParser<R>::RequestParser(std::size_t max_headers_size, std::size_t max_body_size, R& reader)
: m_max_headers_size(max_headers_size), m_max_body_size(max_body_size), m_reader(reader) {
    m_state = State::PARSE_REQUEST_LINE;
    m_current_headers_size = 0;
}

template<Reader R>
asio::awaitable<std::optional<ParseResult>> RequestParser<R>::next() {
    while (m_state != State::PARSE_COMPLETE) {
        std::optional<RequestParserError> result;

        switch (m_state) {
        case State::PARSE_REQUEST_LINE:
            result = co_await parse_request_line();
            break;
        case State::PARSE_HEADERS:
            result = co_await parse_headers();
            break;
        case State::PARSE_BODY_CHUNKED_SIZE:
            result = co_await parse_body_chunked_size();
            break;
        case State::PARSE_BODY_CHUNKED_DATA:
            result = co_await parse_body_chunked_data();
            break;
        case State::PARSE_CHUNKED_TRAILERS:
            result = co_await parse_chunked_trailers();
            break;
        case State::PARSE_BODY_CONTENT_LENGTH:
            result = co_await parse_body_content_length();
            break;
        default:
            break;
        }

        if (result.has_value()) {
            reset_err();

            // If the reader is closed, return nullopt to indicate end of stream
            if (*result == RequestParserError::READER_CLOSED) {
                co_return std::nullopt;
            } else {
                co_return std::unexpected(*result);
            }
        }
    }

    // Create new buffer with remaining data
    std::string remaining_data = std::string(m_view.data(), m_view.size());
    m_buffer = std::move(remaining_data);
    m_view_start = 0;
    m_view = std::string_view(m_buffer.data(), m_buffer.size());

    auto req = m_req;
    reset();
    co_return req;
}

template<Reader R>
asio::awaitable<std::optional<RequestParserError>> RequestParser<R>::parse_request_line() {
    std::size_t crlf;
    while (true) {
        auto crlf_opt = co_await pull_until_crlf();
        if (!crlf_opt.has_value()) {
            co_return RequestParserError::READER_CLOSED;
        }
        crlf = crlf_opt.value();

        if (crlf == 0) {
            // https://www.rfc-editor.org/rfc/rfc9112.html#section-2.2-6
            // A server SHOULD ignore at least one empty line (CRLF)
            // received prior to the request-line
            advance_view(2);
        } else {
            break;
        }
    }

    std::string_view request_line = m_view.substr(0, crlf);

    m_current_headers_size += request_line.size() + 2;
    if (m_current_headers_size > m_max_headers_size) {
        co_return RequestParserError::HEADER_TOO_LARGE;
    }

    auto method_end = request_line.find(' ');
    if (method_end == std::string::npos) {
        co_return RequestParserError::INVALID_REQUEST_LINE;
    }
    m_req.method = request_line.substr(0, method_end);
    if (!valid_token(m_req.method)) {
        co_return RequestParserError::INVALID_REQUEST_LINE;
    }

    auto uri_start = method_end + 1;
    auto uri_end = request_line.find(' ', uri_start);
    if (uri_end == std::string::npos) {
        co_return RequestParserError::INVALID_REQUEST_LINE;
    }

    auto uri_str = request_line.substr(uri_start, uri_end - uri_start);
    auto uri = URI::parse(uri_str);
    if (!uri.has_value()) {
        co_return RequestParserError::INVALID_REQUEST_LINE;
    }
    m_req.uri = *uri;

    auto version_start = uri_end + 1;
    auto version = request_line.substr(version_start);
    if (version != "HTTP/1.1") {
        co_return RequestParserError::INVALID_REQUEST_LINE;
    }

    // Remove the request line and CRLF from the buffer
    advance_view(crlf + 2);
    m_state = State::PARSE_HEADERS;

    co_return std::nullopt;
}

template<Reader R>
asio::awaitable<std::optional<RequestParserError>> RequestParser<R>::parse_headers() {
    while (!m_view.empty()) {
        auto crlf_opt = co_await pull_until_crlf();
        if (!crlf_opt.has_value()) {
            co_return RequestParserError::READER_CLOSED;
        }
        auto crlf = crlf_opt.value();

        std::string_view header_line = m_view.substr(0, crlf);

        m_current_headers_size += header_line.size() + 2;
        if (m_current_headers_size > m_max_headers_size) {
            co_return RequestParserError::HEADER_TOO_LARGE;
        }

        // End of headers
        if (header_line.empty()) {
            advance_view(crlf + 2);
            co_return co_await prepare_parse_body();
        }

        auto result = co_await parse_header(header_line, m_req.headers);
        advance_view(crlf + 2);

        if (result.has_value()) {
            co_return result;
        }
    }

    co_return std::nullopt;
}

template<Reader R>
asio::awaitable<std::optional<RequestParserError>> RequestParser<R>::parse_chunked_trailers() {
    while (!m_view.empty()) {
        auto crlf_opt = co_await pull_until_crlf();
        if (!crlf_opt.has_value()) {
            co_return RequestParserError::READER_CLOSED;
        }
        auto crlf = crlf_opt.value();

        std::string_view header_line = m_view.substr(0, crlf);

        // End of trailers
        if (header_line.empty()) {
            advance_view(crlf + 2);
            m_state = State::PARSE_COMPLETE;
            co_return std::nullopt;
        }

        auto result = co_await parse_header(header_line, m_req.trailers);
        advance_view(crlf + 2);

        if (result.has_value()) {
            co_return result;
        }
    }

    co_return std::nullopt;
}

template<Reader R>
asio::awaitable<std::optional<RequestParserError>>
    RequestParser<R>::parse_header(std::string_view header_line, Headers& target) {
    auto colon_pos = header_line.find(':');
    if (colon_pos == std::string::npos) {
        co_return RequestParserError::INVALID_HEADER;
    }
    std::string_view name = header_line.substr(0, colon_pos);
    if (!valid_token(name)) {
        co_return RequestParserError::INVALID_HEADER;
    }

    // Skip the colon and optional spaces
    std::size_t value_start = colon_pos + 1;
    while (value_start < header_line.size()
           && (header_line[value_start] == ' ' || header_line[value_start] == '\t')) {
        value_start++;
    }

    std::string_view value = header_line.substr(value_start);
    // Trim trailing spaces
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
        value.remove_suffix(1);
    }

    if (name == "Cookie") {
        if (!valid_header_value(value)) {
            co_return RequestParserError::INVALID_HEADER;
        }

        m_req.cookies.push_back(std::string(value));
    }

    try {
        target.set(std::move(name), std::move(value));
    } catch (const std::invalid_argument& e) {
        co_return RequestParserError::INVALID_HEADER;
    }
    co_return std::nullopt;
}

template<Reader R>
asio::awaitable<std::optional<RequestParserError>> RequestParser<R>::prepare_parse_body() {
    auto encoding_opt = m_req.header("Transfer-Encoding");
    auto content_length_opt = m_req.header("Content-Length");

    // Both Content-Length and Transfer-Encoding present
    if (content_length_opt.has_value() && encoding_opt.has_value()) {
        co_return RequestParserError::INVALID_HEADER;
    }

    if (encoding_opt.has_value()) {
        // Only chunked encoding is supported
        if (*encoding_opt != "chunked") {
            co_return RequestParserError::UNSUPPORTED_TRANSFER_ENCODING;
        }

        m_state = State::PARSE_BODY_CHUNKED_SIZE;
    } else if (content_length_opt.has_value()) {
        m_state = State::PARSE_BODY_CONTENT_LENGTH;
    } else {
        // No body
        m_state = State::PARSE_COMPLETE;
    }

    co_return std::nullopt;
}

template<Reader R>
asio::awaitable<std::optional<RequestParserError>> RequestParser<R>::parse_body_content_length() {
    // We know Content-Length is present because of the state machine
    auto content_length_sv = *m_req.header("Content-Length");
    std::size_t content_length = 0;

    auto res = std::from_chars(
        content_length_sv.data(), content_length_sv.data() + content_length_sv.size(),
        content_length
    );
    if (res.ec != std::errc()) {
        co_return RequestParserError::INVALID_HEADER;
    }
    if (content_length > m_max_body_size) {
        co_return RequestParserError::CONTENT_TOO_LARGE;
    }

    while (m_view.size() < content_length) {
        auto data_opt = co_await m_reader.pull();
        if (!data_opt.has_value()) {
            co_return RequestParserError::READER_CLOSED;
        }
        m_buffer.append(std::string(*data_opt));
        m_view = std::string_view(m_buffer.data() + m_view_start, m_buffer.size() - m_view_start);
    }

    m_req.body = m_view.substr(0, content_length);
    advance_view(content_length);
    m_state = State::PARSE_COMPLETE;
    co_return std::nullopt;
}

template<Reader R>
asio::awaitable<std::optional<RequestParserError>> RequestParser<R>::parse_body_chunked_size() {
    auto crlf_opt = co_await pull_until_crlf();
    if (!crlf_opt.has_value()) {
        co_return RequestParserError::READER_CLOSED;
    }
    auto crlf = crlf_opt.value();

    std::string_view size_line = m_view.substr(0, crlf);
    std::size_t chunk_size = 0;
    auto res =
        std::from_chars(size_line.data(), size_line.data() + size_line.size(), chunk_size, 16);
    if (res.ec != std::errc()) {
        co_return RequestParserError::INVALID_CHUNK_ENCODING;
    }
    if (chunk_size > m_max_body_size || m_req.body.size() + chunk_size > m_max_body_size) {
        co_return RequestParserError::CONTENT_TOO_LARGE;
    }

    m_chunk_bytes_remaining = chunk_size;
    m_state = State::PARSE_BODY_CHUNKED_DATA;
    advance_view(crlf + 2);

    co_return std::nullopt;
}

template<Reader R>
asio::awaitable<std::optional<RequestParserError>> RequestParser<R>::parse_body_chunked_data() {
    while (m_view.size() < m_chunk_bytes_remaining + 2) {
        auto data_opt = co_await m_reader.pull();
        if (!data_opt.has_value()) {
            co_return RequestParserError::READER_CLOSED;
        }
        m_buffer.append(std::string(*data_opt));
        m_view = std::string_view(m_buffer.data() + m_view_start, m_buffer.size() - m_view_start);
    }

    // Last chunk
    if (m_chunk_bytes_remaining == 0) {
        m_state = State::PARSE_CHUNKED_TRAILERS;
        co_return std::nullopt;
    }

    m_req.body.append(m_view.substr(0, m_chunk_bytes_remaining));
    advance_view(m_chunk_bytes_remaining);
    m_chunk_bytes_remaining = 0;

    // Expecting CRLF after chunk data
    if (!m_view.starts_with("\r\n")) {
        co_return RequestParserError::INVALID_CHUNK_ENCODING;
    }
    advance_view(2);

    m_state = State::PARSE_BODY_CHUNKED_SIZE;
    co_return std::nullopt;
}

template<Reader R>
void RequestParser<R>::reset() {
    m_req = Request();
    m_state = State::PARSE_REQUEST_LINE;
    m_current_headers_size = 0;
}

template<Reader R>
void RequestParser<R>::reset_err() {
    reset();
    m_buffer = {};
    m_view = {};
    m_view_start = 0;
}

template<Reader R>
void RequestParser<R>::advance_view(std::size_t n) {
    m_view_start += n;
    m_view = std::string_view(m_buffer.data() + m_view_start, m_buffer.size() - m_view_start);
}

template<Reader R>
asio::awaitable<std::optional<std::size_t>> RequestParser<R>::pull_until_crlf() {
    while (true) {
        auto crlf = m_view.find("\r\n");
        if (crlf != std::string::npos) {
            co_return crlf;
        }

        auto data_opt = co_await m_reader.pull();
        if (!data_opt.has_value()) {
            co_return std::nullopt;
        }

        m_buffer.append(std::string(*data_opt));
        m_view = std::string_view(m_buffer.data() + m_view_start, m_buffer.size() - m_view_start);
    }
}

}
