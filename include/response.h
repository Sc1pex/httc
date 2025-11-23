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

    // Blocking versions
    void begin_stream();
    void stream_chunk(std::string_view chunk);
    void end_stream();

    // Async versions
    asio::awaitable<void> async_begin_stream();
    asio::awaitable<void> async_stream_chunk(std::string_view chunk);
    asio::awaitable<void> async_end_stream();

    void set_body(std::string_view body);

    static asio::awaitable<void> send(Response&& res);
    // Needed because co_await is not avabile in catch
    static void send_blocking(Response&& res);

    StatusCode status;
    Headers headers;

private:
    asio::ip::tcp::socket& m_sock;
    std::string m_body;
    bool m_head;
    State m_state;
};

}
