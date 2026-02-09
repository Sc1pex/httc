#pragma once

#include "httc/headers.hpp"
#include "httc/io.hpp"
#include "httc/status.hpp"

namespace httc {

class Response {
    using WriteFn = asio::awaitable<void>(*)(void*, std::vector<asio::const_buffer>);

public:
    template<Writer W>
    Response(W& writer, bool is_head_response = false)
    : m_writer_ptr(&writer),
      m_write_fn([](void* w, std::vector<asio::const_buffer> b) -> asio::awaitable<void> {
          return static_cast<W*>(w)->write(std::move(b));
      }),
      m_head(is_head_response) {
        status = StatusCode::OK;
        headers.set("Content-Length", "0");
        m_state = State::Uninitialized;
    }

    static Response from_status(SocketWriter& writer, StatusCode status);

    class ChunkedStream {
    public:
        // If the chunk is not empty, writes immediately
        // Otherwise does nothing
        asio::awaitable<void> write(std::string_view chunk);

        // Sends 0 sized chunk to end the stream
        asio::awaitable<void> end();

    private:
        friend class Response;
        Response& m_parent;
        ChunkedStream(Response& parent) : m_parent(parent) {
        }
    };

    class FixedStream {
    public:
        asio::awaitable<void> write(std::string_view data);

    private:
        friend class Response;
        Response& m_parent;
        FixedStream(Response& parent) : m_parent(parent) {
        }
    };

    // Send request head with Transfer-Encoding: chunked
    // Returns a writer to push more chunks
    asio::awaitable<ChunkedStream> send_chunked();

    // Send request head with Content-Length set
    // Returns a writer to push data in packets
    asio::awaitable<FixedStream> send_fixed(std::size_t content_size);

    void add_cookie(std::string cookie);

    void set_body(std::string_view body);

    // Should not be called by handlers
    asio::awaitable<void> send();

    StatusCode status;
    Headers headers;
    std::vector<std::string> cookies;

    bool is_head() {
        return m_head;
    }

private:
    enum class State {
        Uninitialized,
        StreamChunk,
        StreamFixed,
        Body,
        Sent,
    };

    void generate_head();
    asio::awaitable<void> write_to_writer(std::vector<asio::const_buffer> buffers);

private:
    void* m_writer_ptr;
    WriteFn m_write_fn;

    std::string m_body;
    bool m_head;
    State m_state;

    std::string m_head_buffer;
};
}
