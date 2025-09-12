#pragma once

#include <format>
#include <optional>
#include <string>
#include <string_view>
#include "httc/headers.h"
#include "httc/uri.h"

namespace httc {

class Request {
public:
    Request();

    std::optional<std::string_view> header(std::string_view header) const;

    std::string method;
    URI uri;
    std::string body;
    Headers headers;
};

}

template<>
struct std::formatter<httc::Request> : std::formatter<std::string> {
    auto format(const httc::Request& req, std::format_context& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "method: {}\n", req.method);
        out = std::format_to(out, "path: {}\n", req.uri);
        out = std::format_to(out, "Headers:\n{}", req.headers);
        out = std::format_to(out, "Body: {}", req.body);
        return out;
    }
};
