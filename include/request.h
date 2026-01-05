#pragma once

#include <format>
#include <string>
#include "headers.h"
#include "reader.h"
#include "uri.h"

namespace httc {

class Request {
public:
    Request();

    std::string method;
    URI uri;
    std::string body;

    Headers headers;
    Headers trailers;
    std::vector<std::string> cookies;

    std::string wildcard_path;
    std::unordered_map<std::string, std::string> path_params;

private:
    std::string m_raw_headers;

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
