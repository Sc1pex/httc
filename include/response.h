#pragma once

#include <asio/awaitable.hpp>
#include <asio/ip/tcp.hpp>
#include "headers.h"
#include "status.h"

namespace httc {

class Response {
public:
    enum class State {
        Uninitialized,
        Stream,
        Body,
    };

    Response(asio::ip::tcp::socket& sock, bool is_head_response = false);
    static Response from_status(asio::ip::tcp::socket& sock, StatusCode status);

    asio::awaitable<void> begin_stream();
    asio::awaitable<void> stream_chunk(std::string_view chunk);
    asio::awaitable<void> end_stream();

    void set_body(std::string_view body);

    static asio::awaitable<void> send(Response&& res);

    void add_cookie(std::string cookie);

    StatusCode status;
    Headers headers;

private:
    asio::ip::tcp::socket& m_sock;
    std::string m_body;
    bool m_head;
    State m_state;

    std::vector<std::string> m_cookies;
};

}
