#pragma once

#include <asio.hpp>
#include <expected>
#include "request.h"
#include "status.h"

namespace httc {

enum class RequestParserError {
    INVALID_REQUEST_LINE,
    INVALID_HEADER,
    UNSUPPORTED_TRANSFER_ENCODING,
    CONTENT_TOO_LARGE,
    INVALID_CHUNK_ENCODING,
};

StatusCode parse_error_to_status_code(RequestParserError error);

class RequestParser {
public:
    RequestParser();
    using ParseResult = std::expected<Request, RequestParserError>;

    std::optional<ParseResult> feed_data(const char* data, std::size_t length);

private:
    enum class State {
        PARSE_REQUEST_LINE,
        PARSE_HEADERS,
        PARSE_BODY_CHUNKED_SIZE,
        PARSE_BODY_CHUNKED_DATA,
        PARSE_BODY_CONTENT_LENGTH,
        PARSE_COMPLETE,
    };

    std::optional<RequestParserError> parse_request_line();
    std::optional<RequestParserError> parse_headers();
    std::optional<RequestParserError> prepare_parse_body();
    std::optional<RequestParserError> parse_body_content_length();
    std::optional<RequestParserError> parse_body_chunked_size();
    std::optional<RequestParserError> parse_body_chunked_data();

    void reset();

    std::optional<RequestParserError> parse_header(const std::string& header_line);

    bool valid_token(const std::string& str);
    bool valid_header_value(const std::string& str);

private:
    Request m_req;
    std::string m_buffer;
    State m_state;

    std::size_t m_chunk_bytes_remaining;
};

}
