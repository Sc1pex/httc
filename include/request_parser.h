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
    HEADER_TOO_LARGE,
    INVALID_CHUNK_ENCODING,
};

StatusCode parse_error_to_status_code(RequestParserError error);

class RequestParser {
public:
    RequestParser(std::size_t max_headers_size, std::size_t max_body_size);
    using ParseResult = std::expected<Request, RequestParserError>;

    std::optional<ParseResult> feed_data(const char* data, std::size_t length);

private:
    enum class State {
        PARSE_REQUEST_LINE,
        PARSE_HEADERS,
        PARSE_BODY_CHUNKED_SIZE,
        PARSE_BODY_CHUNKED_DATA,
        PARSE_CHUNKED_TRAILERS,
        PARSE_BODY_CONTENT_LENGTH,
        PARSE_COMPLETE,
    };

    std::optional<RequestParserError> parse_request_line();
    std::optional<RequestParserError> parse_headers();
    std::optional<RequestParserError> prepare_parse_body();
    std::optional<RequestParserError> parse_body_content_length();
    std::optional<RequestParserError> parse_body_chunked_size();
    std::optional<RequestParserError> parse_body_chunked_data();
    std::optional<RequestParserError> parse_chunked_trailers();

    void reset();
    void reset_err();

    void advance_view(std::size_t n);

    std::optional<RequestParserError> parse_header(std::string_view header_line, Headers& target);

private:
    Request m_req;
    State m_state;

    std::string m_buffer;

    std::string_view m_view;
    std::size_t m_view_start = 0;

    std::size_t m_chunk_bytes_remaining;

    std::size_t m_max_headers_size;
    // Current headers size, including request line and CRLFs
    std::size_t m_current_headers_size;
    std::size_t m_max_body_size;
};

}
