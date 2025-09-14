#pragma once

#include <expected>
#include <uvw.hpp>
#include "httc/request.h"
#include "httc/status.h"

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

    using request_complete_fn = std::function<void(Request&)>;
    using error_fn = std::function<void(RequestParserError)>;

    void set_on_request_complete(request_complete_fn callback);
    void set_on_error(error_fn callback);
    void feed_data(const char* data, std::size_t length);

private:
    enum class State {
        PARSE_REQUEST_LINE,
        PARSE_HEADERS,
        PARSE_BODY_CHUNKED_SIZE,
        PARSE_BODY_CHUNKED_DATA,
        PARSE_BODY_CONTENT_LENGTH,
        PARSE_COMPLETE,
    };

    using ParseResult = std::expected<void, RequestParserError>;

    ParseResult parse_request_line();
    ParseResult parse_headers();
    ParseResult prepare_parse_body();
    ParseResult parse_body_content_length();
    ParseResult parse_body_chunked_size();
    ParseResult parse_body_chunked_data();

    void reset();

    ParseResult parse_header(const std::string& header_line);

    bool valid_token(const std::string& str);
    bool valid_header_value(const std::string& str);

private:
    request_complete_fn m_on_request_complete;
    error_fn m_on_error;

    Request m_req;
    std::string m_buffer;
    State m_state;

    std::size_t m_chunk_bytes_remaining;
};

}
