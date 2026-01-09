#pragma once

#include "headers.hpp"
#include "io.hpp"
#include "status.hpp"

namespace httc {

class Response {
public:
    Response(Writer& writer, bool is_head_response = false);
    static Response from_status(Writer& writer, StatusCode status);

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

    void generate_status_line();
    std::vector<asio::const_buffer> response_head();

private:
    Writer& m_writer;

    std::string m_body;
    bool m_head;
    State m_state;

    std::string m_status_line;
};
}
