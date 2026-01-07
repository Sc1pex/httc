#pragma once

#include <asio.hpp>
#include <expected>
#include "reader.h"
#include "request.h"
#include "status.h"

namespace httc {

enum class RequestParserError {
    READER_CLOSED,
    INVALID_REQUEST_LINE,
    INVALID_HEADER,
    UNSUPPORTED_TRANSFER_ENCODING,
    CONTENT_TOO_LARGE,
    HEADER_TOO_LARGE,
    INVALID_CHUNK_ENCODING,
};

StatusCode parse_error_to_status_code(RequestParserError error);

using ParseResult = std::expected<Request, RequestParserError>;

template<Reader R = SocketReader>
class RequestParser {
public:
    RequestParser(std::size_t max_headers_size, std::size_t max_body_size, R& reader);

    asio::awaitable<std::optional<ParseResult>> next();

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

    asio::awaitable<std::optional<RequestParserError>> parse_request_line();
    asio::awaitable<std::optional<RequestParserError>> parse_headers();
    asio::awaitable<std::optional<RequestParserError>> prepare_parse_body();
    asio::awaitable<std::optional<RequestParserError>> parse_body_content_length();
    asio::awaitable<std::optional<RequestParserError>> parse_body_chunked_size();
    asio::awaitable<std::optional<RequestParserError>> parse_body_chunked_data();
    asio::awaitable<std::optional<RequestParserError>> parse_chunked_trailers();

    void reset();
    void reset_err();

    void advance_view(std::size_t n);

    asio::awaitable<std::expected<std::size_t, RequestParserError>> pull_until(
        std::string_view chars, std::size_t max_size,
        RequestParserError overflow_error = RequestParserError::HEADER_TOO_LARGE
    );

    asio::awaitable<std::optional<RequestParserError>>
        parse_header(std::string_view header_line, Headers& target);

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

    R& m_reader;
};

}

#include "request_parser.tpp"
