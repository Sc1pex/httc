#include "httc/response.h"
#include <asio.hpp>
#include <asio/error_code.hpp>
#include "httc/status.h"

namespace httc {

using asio::ip::tcp;

template<typename... Args>
void write_fmt(tcp::socket& client, std::format_string<Args...> fmt, Args&&... args) {
    auto s = std::format(fmt, std::forward<Args>(args)...);
    asio::error_code ec;
    asio::write(client, asio::buffer(s), ec);
}

Response::Response(bool is_head_response) {
    m_head = is_head_response;
    status = StatusCode::OK;
    headers.set("Content-Length", "0");
}

void Response::set_body(std::string_view body) {
    headers.set("Content-Length", std::to_string(body.size()));
    if (!m_head) {
        m_body = std::string(body);
    }
}

void Response::write(tcp::socket& client) {
    write_fmt(client, "HTTP/1.1 {}\r\n", status.code);
    for (const auto& [key, value] : headers) {
        write_fmt(client, "{}: {}\r\n", key, value);
    }
    write_fmt(client, "\r\n");
    if (!m_body.empty()) {
        asio::error_code ec;
        asio::write(client, asio::buffer(m_body), ec);
    }
}

Response Response::from_status(StatusCode status) {
    Response r;
    r.status = status;
    return r;
}

}
