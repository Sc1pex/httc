#include "request_parser.h"
#include <charconv>
#include <expected>
#include <optional>
#include <string_view>
#include "common.h"

namespace httc {

RequestParser::RequestParser(std::size_t max_headers_size, std::size_t max_body_size)
: m_max_headers_size(max_headers_size), m_max_body_size(max_body_size) {
    m_state = State::PARSE_REQUEST_LINE;
    m_current_headers_size = 0;
}

std::optional<RequestParser::ParseResult>
    RequestParser::feed_data(const char* data, std::size_t length) {
    if (m_state == State::PARSE_HEADERS || m_state == State::PARSE_REQUEST_LINE) {
        m_current_headers_size += length;

        if (m_current_headers_size > m_max_headers_size) {
            reset();
            m_buffer = {};
            return std::unexpected(RequestParserError::HEADER_TOO_LARGE);
        }
    }

    m_buffer.append(data, length);

    while (true) {
        auto last_state = m_state;
        bool completed = false;

        if (m_state == State::PARSE_REQUEST_LINE) {
            auto result = parse_request_line();
            if (result.has_value()) {
                reset();
                // Since the request could not be parsed, any data left in the buffer
                // cannot be trusted to be valid
                m_buffer = {};
                return std::unexpected(result.value());
            }
        } else if (m_state == State::PARSE_HEADERS) {
            auto result = parse_headers();
            if (result.has_value()) {
                reset();
                m_buffer = {};
                return std::unexpected(result.value());
            }
        } else if (m_state == State::PARSE_BODY_CHUNKED_SIZE) {
            auto result = parse_body_chunked_size();
            if (result.has_value()) {
                reset();
                m_buffer = {};
                return std::unexpected(result.value());
            }
        } else if (m_state == State::PARSE_BODY_CHUNKED_DATA) {
            auto result = parse_body_chunked_data();
            if (result.has_value()) {
                reset();
                m_buffer = {};
                return std::unexpected(result.value());
            }
        } else if (m_state == State::PARSE_BODY_CONTENT_LENGTH) {
            auto result = parse_body_content_length();
            if (result.has_value()) {
                reset();
                m_buffer = {};
                return std::unexpected(result.value());
            }
        } else if (m_state == State::PARSE_COMPLETE) {
            auto req = m_req;
            reset();
            completed = true;
            return req;
        }

        if (last_state == m_state && !completed) {
            // No progress made, need more data
            return std::nullopt;
        }
    }
}

std::optional<RequestParserError> RequestParser::parse_request_line() {
    // https://www.rfc-editor.org/rfc/rfc9112.html#section-2.2-6
    // A server SHOULD ignore at least one empty line (CRLF)
    // received prior to the request-line
    while (m_buffer.starts_with("\r\n")) {
        m_buffer.erase(0, 2);
    }

    auto crlf = m_buffer.find("\r\n");
    if (crlf == std::string::npos) {
        // Not enough data yet
        return std::nullopt;
    }

    std::string request_line = m_buffer.substr(0, crlf);
    // Remove the request line and CRLF from the buffer
    m_buffer.erase(0, crlf + 2);

    auto method_end = request_line.find(' ');
    if (method_end == std::string::npos) {
        return RequestParserError::INVALID_REQUEST_LINE;
    }
    m_req.method = request_line.substr(0, method_end);
    if (!valid_token(m_req.method)) {
        return RequestParserError::INVALID_REQUEST_LINE;
    }

    auto uri_start = method_end + 1;
    auto uri_end = request_line.find(' ', uri_start);
    if (uri_end == std::string::npos) {
        return RequestParserError::INVALID_REQUEST_LINE;
    }

    auto uri_str = request_line.substr(uri_start, uri_end - uri_start);
    auto uri = URI::parse(uri_str);
    if (!uri.has_value()) {
        return RequestParserError::INVALID_REQUEST_LINE;
    }
    m_req.uri = *uri;

    auto version_start = uri_end + 1;
    auto version = request_line.substr(version_start);
    if (version != "HTTP/1.1") {
        return RequestParserError::INVALID_REQUEST_LINE;
    }

    m_state = State::PARSE_HEADERS;

    return std::nullopt;
}

std::optional<RequestParserError> RequestParser::parse_headers() {
    while (!m_buffer.empty()) {
        auto crlf = m_buffer.find("\r\n");
        if (crlf == std::string::npos) {
            // Not enough data yet
            return std::nullopt;
        }

        std::string header_line = m_buffer.substr(0, crlf);
        m_buffer.erase(0, crlf + 2);

        // End of headers
        if (header_line.empty()) {
            return prepare_parse_body();
        }

        auto result = parse_header(header_line);
        if (result.has_value()) {
            return result;
        }
    }

    return std::nullopt;
}

std::optional<RequestParserError> RequestParser::parse_header(const std::string& header_line) {
    auto colon_pos = header_line.find(':');
    if (colon_pos == std::string::npos) {
        return RequestParserError::INVALID_HEADER;
    }
    std::string name = header_line.substr(0, colon_pos);
    if (!valid_token(name)) {
        return RequestParserError::INVALID_HEADER;
    }

    // Skip the colon and optional spaces
    std::size_t value_start = colon_pos + 1;
    while (value_start < header_line.size()
           && (header_line[value_start] == ' ' || header_line[value_start] == '\t')) {
        value_start++;
    }

    std::string value = header_line.substr(value_start);
    // Trim trailing spaces
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
        value.pop_back();
    }

    try {
        m_req.headers.set(std::move(name), std::move(value));
    } catch (const std::invalid_argument& e) {
        return RequestParserError::INVALID_HEADER;
    }
    return std::nullopt;
}

std::optional<RequestParserError> RequestParser::prepare_parse_body() {
    auto encoding_opt = m_req.header("Transfer-Encoding");
    auto content_length_opt = m_req.header("Content-Length");

    // Both Content-Length and Transfer-Encoding present
    if (content_length_opt.has_value() && encoding_opt.has_value()) {
        return RequestParserError::INVALID_HEADER;
    }

    if (encoding_opt.has_value()) {
        // Only chunked encoding is supported
        if (*encoding_opt != "chunked") {
            return RequestParserError::UNSUPPORTED_TRANSFER_ENCODING;
        }

        m_state = State::PARSE_BODY_CHUNKED_SIZE;
    } else if (content_length_opt.has_value()) {
        m_state = State::PARSE_BODY_CONTENT_LENGTH;
    } else {
        // No body
        m_state = State::PARSE_COMPLETE;
    }

    return std::nullopt;
}

std::optional<RequestParserError> RequestParser::parse_body_content_length() {
    // We know Content-Length is present because of the state machine
    auto content_length_sv = *m_req.header("Content-Length");
    std::size_t content_length = 0;

    auto res = std::from_chars(
        content_length_sv.data(), content_length_sv.data() + content_length_sv.size(),
        content_length
    );
    if (res.ec != std::errc()) {
        return RequestParserError::INVALID_HEADER;
    }
    if (content_length > m_max_body_size) {
        return RequestParserError::CONTENT_TOO_LARGE;
    }

    if (m_buffer.size() < content_length) {
        // Not enough data yet
        return std::nullopt;
    }

    m_req.body = m_buffer.substr(0, content_length);
    m_buffer.erase(0, content_length);
    m_state = State::PARSE_COMPLETE;
    return std::nullopt;
}

std::optional<RequestParserError> RequestParser::parse_body_chunked_size() {
    auto crlf = m_buffer.find("\r\n");
    if (crlf == std::string::npos) {
        // Not enough data yet
        return std::nullopt;
    }

    std::string size_line = m_buffer.substr(0, crlf);
    m_buffer.erase(0, crlf + 2);
    std::size_t chunk_size = 0;
    auto res =
        std::from_chars(size_line.data(), size_line.data() + size_line.size(), chunk_size, 16);
    if (res.ec != std::errc()) {
        return RequestParserError::INVALID_CHUNK_ENCODING;
    }
    if (chunk_size > m_max_body_size || m_req.body.size() + chunk_size > m_max_body_size) {
        return RequestParserError::CONTENT_TOO_LARGE;
    }

    m_chunk_bytes_remaining = chunk_size;
    m_state = State::PARSE_BODY_CHUNKED_DATA;

    return std::nullopt;
}

std::optional<RequestParserError> RequestParser::parse_body_chunked_data() {
    if (m_buffer.size() < m_chunk_bytes_remaining + 2) {
        // Not enough data yet
        return std::nullopt;
    }

    // Last chunk
    if (m_chunk_bytes_remaining == 0) {
        m_state = State::PARSE_COMPLETE;
        return std::nullopt;
    }

    m_req.body.append(m_buffer.substr(0, m_chunk_bytes_remaining));
    m_buffer.erase(0, m_chunk_bytes_remaining);
    m_chunk_bytes_remaining = 0;

    // Expecting CRLF after chunk data
    if (!m_buffer.starts_with("\r\n")) {
        return RequestParserError::INVALID_CHUNK_ENCODING;
    }
    m_buffer.erase(0, 2);

    m_state = State::PARSE_BODY_CHUNKED_SIZE;
    return std::nullopt;
}

void RequestParser::reset() {
    m_req = Request();
    m_state = State::PARSE_REQUEST_LINE;
}

StatusCode parse_error_to_status_code(RequestParserError error) {
    if (error == RequestParserError::UNSUPPORTED_TRANSFER_ENCODING) {
        return StatusCode::NOT_IMPLEMENTED;
    }
    if (error == RequestParserError::CONTENT_TOO_LARGE) {
        return StatusCode::PAYLOAD_TOO_LARGE;
    }

    return StatusCode::BAD_REQUEST;
}
}
