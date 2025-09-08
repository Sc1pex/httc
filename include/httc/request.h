#pragma once

#include <format>
#include <optional>
#include <string>
#include <string_view>
#include "httc/headers.h"

namespace httc {

class Request {
public:
    std::optional<std::string_view> header(std::string_view header) const;
    std::string_view method() const;
    std::string_view uri() const;
    std::string_view body() const;

private:
    std::string m_method;
    std::string m_uri;
    std::string m_body;
    Headers m_headers;

    friend class RequestParser;
    friend struct std::formatter<Request>;
};

}

template<>
struct std::formatter<httc::Request> : std::formatter<std::string> {
    auto format(const httc::Request& req, std::format_context& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "method: {}\n", req.m_method);
        out = std::format_to(out, "path: {}\n", req.m_uri);
        out = std::format_to(out, "Headers:\n{}", req.m_headers);
        out = std::format_to(out, "Body: {}", req.m_body);
        return out;
    }
};
