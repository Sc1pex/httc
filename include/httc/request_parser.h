#pragma once

#include <expected>
#include <uvw.hpp>
#include "httc/request.h"

namespace httc {

enum class RequestParserError {
    INVALID_REQUEST_LINE,
    INVALID_HEADER,
};

class RequestParser {
public:
    using request_complete_fn = std::function<void(const Request&)>;
    using error_fn = std::function<void(RequestParserError)>;

    void set_on_request_complete(request_complete_fn callback);
    void set_on_error(error_fn callback);
    void feed_data(const char* data, std::size_t length);

private:
    enum class State {
        PARSE_REQUEST_LINE,
        PARSE_HEADERS,
        PARSE_BODY,
        PARSE_COMPLETE,
    };

    std::expected<void, RequestParserError> parse_request_line();
    std::expected<void, RequestParserError> parse_headers();
    std::expected<void, RequestParserError> parse_header(const std::string& header_line);
    std::expected<void, RequestParserError> parse_body();
    void prepare_parse_body();

    void reset();

    bool valid_token(const std::string& str);
    bool valid_header_value(const std::string& str);

private:
    request_complete_fn m_on_request_complete;
    error_fn m_on_error;

    Request m_req;
    std::string m_buffer;
    State m_state;
};

}
