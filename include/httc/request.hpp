#pragma once

#include <asio/any_io_executor.hpp>
#include <format>
#include <optional>
#include <string>
#include <unordered_map>
#include "httc/headers.hpp"
#include "httc/io.hpp"
#include "httc/uri.hpp"

namespace httc {

class Router;
struct ServerConfig;

class Request {
public:
    Request();

    Request(const Request&) = delete;
    Request& operator=(const Request&) = delete;

    Request(Request&&) noexcept = default;
    Request& operator=(Request&&) noexcept = default;
    ~Request() = default;

    std::string method;
    URI uri;
    std::string body;

    Headers headers;
    Headers trailers;
    std::unordered_map<std::string_view, std::string_view> cookies;

    std::string wildcard_path;
    std::unordered_map<std::string, std::string> path_params;

    asio::any_io_executor thread_pool_executor() const {
        if (!m_thread_pool_executor) {
            throw std::runtime_error("Thread pool executor not set");
        }
        return *m_thread_pool_executor;
    }

private:
    void set_thread_pool_executor(asio::any_io_executor ex) {
        m_thread_pool_executor = std::move(ex);
    }

    // NOLINTNEXTLINE(modernize-avoid-c-arrays,cppcoreguidelines-avoid-c-arrays)
    std::unique_ptr<char[]> m_raw_headers;
    std::optional<asio::any_io_executor> m_thread_pool_executor;

    friend asio::awaitable<void> handle_conn(
        asio::ip::tcp::socket socket, std::shared_ptr<Router> router, ServerConfig cfg,
        asio::any_io_executor thread_pool_executor
    );

    template<Reader R>
    friend class RequestParser;
};

}

template<>
struct std::formatter<httc::Request> : std::formatter<std::string> {
    auto static format(const httc::Request& req, std::format_context& ctx) {
        auto out = ctx.out();
        out = std::format_to(out, "method: {}\n", req.method);
        out = std::format_to(out, "path: {}\n", req.uri);
        out = std::format_to(out, "Headers:\n{}", req.headers);
        out = std::format_to(out, "Trailers:\n{}", req.trailers);
        out = std::format_to(out, "Body: {}", req.body);
        return out;
    }
};
