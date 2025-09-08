#include "httc/request_parser.h"
#include <charconv>
#include <expected>
#include <functional>
#include "httc/status.h"

namespace httc {

const std::size_t MAX_BODY_SIZE = 10 * 1024 * 1024; // 10 MB

RequestParser::RequestParser() {
    m_state = State::PARSE_REQUEST_LINE;
}

void RequestParser::feed_data(const char* data, std::size_t length) {
    m_buffer.append(data, length);

    auto maybe_send_error = [this](ParseResult res) {
        if (!res.has_value()) {
            if (m_on_error) {
                m_on_error(res.error());
            }
            reset();
        }
    };

    if (m_state == State::PARSE_REQUEST_LINE) {
        auto result = parse_request_line();
        maybe_send_error(result);
    }
    if (m_state == State::PARSE_HEADERS) {
        auto result = parse_headers();
        maybe_send_error(result);
    }
    if (m_state == State::PARSE_BODY_CHUNKED) {
        auto result = parse_body_chunked();
        maybe_send_error(result);
    }
    if (m_state == State::PARSE_BODY_CONTENT_LENGTH) {
        auto result = parse_body_content_length();
        maybe_send_error(result);
    }
    if (m_state == State::PARSE_COMPLETE) {
        if (m_on_request_complete) {
            m_on_request_complete(m_req);
        }
        reset();
    }
}

void RequestParser::set_on_request_complete(request_complete_fn callback) {
    m_on_request_complete = callback;
}

void RequestParser::set_on_error(error_fn callback) {
    m_on_error = callback;
}

RequestParser::ParseResult RequestParser::parse_request_line() {
    // https://www.rfc-editor.org/rfc/rfc9112.html#section-2.2-6
    // A server SHOULD ignore at least one empty line (CRLF)
    // received prior to the request-line
    while (m_buffer.starts_with("\r\n")) {
        m_buffer.erase(0, 2);
    }

    auto crlf = m_buffer.find("\r\n");
    if (crlf == std::string::npos) {
        // Not enough data yet
        return {};
    }

    std::string request_line = m_buffer.substr(0, crlf);
    // Remove the request line and CRLF from the buffer
    m_buffer.erase(0, crlf + 2);

    auto method_end = request_line.find(' ');
    if (method_end == std::string::npos) {
        return std::unexpected(RequestParserError::INVALID_REQUEST_LINE);
    }
    m_req.m_method = request_line.substr(0, method_end);
    if (!valid_token(m_req.m_method)) {
        return std::unexpected(RequestParserError::INVALID_REQUEST_LINE);
    }

    auto uri_start = method_end + 1;
    auto uri_end = request_line.find(' ', uri_start);
    if (uri_end == std::string::npos) {
        return std::unexpected(RequestParserError::INVALID_REQUEST_LINE);
    }

    // TODO: Validate URI
    m_req.m_uri = request_line.substr(uri_start, uri_end - uri_start);

    auto version_start = uri_end + 1;
    auto version = request_line.substr(version_start);
    if (version != "HTTP/1.1") {
        return std::unexpected(RequestParserError::INVALID_REQUEST_LINE);
    }

    m_state = State::PARSE_HEADERS;

    return {};
}

RequestParser::ParseResult RequestParser::parse_headers() {
    auto end = m_buffer.find("\r\n\r\n");
    if (end == std::string::npos) {
        // Not enough data yet
        return {};
    }

    // Extract the headers block (up to the last CRLF before the CRLFCRLF)
    std::string headers_block = m_buffer.substr(0, end + 2);
    m_buffer.erase(0, end + 4);

    while (!headers_block.empty()) {
        auto crlf = headers_block.find("\r\n");
        std::string header_line = headers_block.substr(0, crlf);
        headers_block.erase(0, crlf + 2);

        auto result = parse_header(header_line);
        if (!result.has_value()) {
            return result;
        }
    }

    return prepare_parse_body();
}

RequestParser::ParseResult RequestParser::parse_header(const std::string& header_line) {
    auto colon_pos = header_line.find(':');
    if (colon_pos == std::string::npos) {
        return std::unexpected(RequestParserError::INVALID_HEADER);
    }
    std::string name = header_line.substr(0, colon_pos);
    if (!valid_token(name)) {
        return std::unexpected(RequestParserError::INVALID_HEADER);
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

    if (!valid_header_value(value)) {
        return std::unexpected(RequestParserError::INVALID_HEADER);
    }

    m_req.m_headers[name] = value;
    return {};
}

RequestParser::ParseResult RequestParser::prepare_parse_body() {
    auto encoding_opt = m_req.header("Transfer-Encoding");
    auto content_length_opt = m_req.header("Content-Length");

    // Both Content-Length and Transfer-Encoding present
    if (content_length_opt.has_value() && encoding_opt.has_value()) {
        return std::unexpected(RequestParserError::INVALID_HEADER);
    }

    if (encoding_opt.has_value()) {
        // Only chunked encoding is supported
        if (*encoding_opt != "chunked") {
            return std::unexpected(RequestParserError::UNSUPPORTED_TRANSFER_ENCODING);
        }

        m_state = State::PARSE_BODY_CHUNKED;
    } else if (content_length_opt.has_value()) {
        m_state = State::PARSE_BODY_CONTENT_LENGTH;
    } else {
        // No body
        m_state = State::PARSE_COMPLETE;
    }

    return {};
}

RequestParser::ParseResult RequestParser::parse_body_chunked() {
    return {};
}

RequestParser::ParseResult RequestParser::parse_body_content_length() {
    // We know Content-Length is present because of the state machine
    auto content_length_sv = *m_req.header("Content-Length");
    std::size_t content_length = 0;

    auto res = std::from_chars(
        content_length_sv.data(), content_length_sv.data() + content_length_sv.size(),
        content_length
    );
    if (res.ec != std::errc()) {
        return std::unexpected(RequestParserError::INVALID_HEADER);
    }
    if (content_length > MAX_BODY_SIZE) {
        return std::unexpected(RequestParserError::CONTENT_TOO_LARGE);
    }

    if (m_buffer.size() < content_length) {
        // Not enough data yet
        return {};
    }

    m_req.m_body = m_buffer.substr(0, content_length);
    m_buffer.erase(0, content_length);
    m_state = State::PARSE_COMPLETE;
    return {};
}

void RequestParser::reset() {
    m_req = Request();
    m_buffer.clear();
    m_state = State::PARSE_REQUEST_LINE;
}

// https://www.rfc-editor.org/rfc/rfc9110#name-tokens
bool RequestParser::valid_token(const std::string& str) {
    for (char c : str) {
        if (c == '!' || c == '#' || c == '$' || c == '%' || c == '&' || c == '\'' || c == '*'
            || c == '+' || c == '-' || c == '.' || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')
            || c == '^' || c == '_' || c == '`' || (c >= 'a' && c <= 'z') || c == '|' || c == '~') {
            continue;
        }
        return false;
    }
    return true;
}

bool RequestParser::valid_header_value(const std::string& str) {
    for (char c : str) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc == 0x09 || (uc >= 0x20 && uc <= 0x7E) || (uc >= 0x80 && uc <= 0xFF)) {
            continue;
        }
        return false;
    }
    return true;
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
