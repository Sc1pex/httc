#pragma once

#include "headers.hpp"
#include "io.hpp"
#include "status.hpp"

namespace httc {

class Response {
public:
    Response(Writer& writer, bool is_head_response = false);
    static Response from_status(Writer& writer, StatusCode status);

    asio::awaitable<void> begin_stream();
    asio::awaitable<void> stream_chunk(std::string_view chunk);
    asio::awaitable<void> end_stream();

    void set_body(std::string_view body);

    static asio::awaitable<void> send(Response&& res);

    void add_cookie(std::string cookie);

    StatusCode status;
    Headers headers;

private:
    enum class State {
        Uninitialized,
        Stream,
        Body,
    };

    void generate_status_line();
    std::vector<asio::const_buffer> response_head();

private:
    Writer& m_writer;

    std::string m_body;
    bool m_head;
    State m_state;

    std::string m_status_line;

    std::vector<std::string> m_cookies;
};

}
