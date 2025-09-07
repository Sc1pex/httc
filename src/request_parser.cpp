#include "httc/request_parser.h"

namespace httc {

void RequestParser::feed_data(const char* data, std::size_t length) {
    m_buffer.append(data, length);

    auto maybe_send_error = [this](std::expected<void, RequestParserError> res) {
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
    if (m_state == State::PARSE_BODY) {
        auto result = parse_body();
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

std::expected<void, RequestParserError> RequestParser::parse_request_line() {
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

std::expected<void, RequestParserError> RequestParser::parse_headers() {
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

    prepare_parse_body();
    return {};
}

std::expected<void, RequestParserError>
    RequestParser::parse_header(const std::string& header_line) {
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

std::expected<void, RequestParserError> RequestParser::parse_body() {
    return {};
}

void RequestParser::prepare_parse_body() {
    // TODO: Check for Content-Length or Transfer-Encoding headers
    m_state = State::PARSE_COMPLETE;
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

}
