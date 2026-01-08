#pragma once

#include <format>
#include <string>
#include <unordered_map>
#include "headers.hpp"
#include "reader.hpp"
#include "uri.hpp"

namespace httc {

class Request {
public:
    Request();

    Request(const Request&) = delete;
    Request& operator=(const Request&) = delete;

    Request(Request&&) noexcept = default;
    Request& operator=(Request&&) noexcept = default;

    std::string method;
    URI uri;
    std::string body;

    Headers headers;
    Headers trailers;
    std::unordered_map<std::string_view, std::string_view> cookies;

    std::string wildcard_path;
    std::unordered_map<std::string, std::string> path_params;

private:
    std::unique_ptr<char[]> m_raw_headers;

    template<Reader R>
    friend class RequestParser;
};

}

template<>
struct std::formatter<httc::Request> : std::formatter<std::string> {
    auto format(const httc::Request& req, std::format_context& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "method: {}\n", req.method);
        out = std::format_to(out, "path: {}\n", req.uri);
        out = std::format_to(out, "Headers:\n{}", req.headers);
        out = std::format_to(out, "Trailers:\n{}", req.trailers);
        out = std::format_to(out, "Body: {}", req.body);
        return out;
    }
};
