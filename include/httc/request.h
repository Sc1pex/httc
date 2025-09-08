#pragma once

#include <format>
#include <optional>
#include <string>
#include <unordered_map>

namespace httc {

class Request {
public:
    std::optional<std::string_view> header(const std::string& header) const;
    std::string_view method() const;
    std::string_view uri() const;

private:
    std::string m_method;
    std::string m_uri;
    std::unordered_map<std::string, std::string> m_headers;
    std::string m_body;

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
        for (const auto& [key, value] : req.m_headers) {
            out = std::format_to(out, "{}: {}\n", key, value);
        }
        out = std::format_to(out, "Body: {}", req.m_body);
        return out;
    }
};
