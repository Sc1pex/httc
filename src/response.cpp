#include "httc/response.h"
#include "httc/status.h"

namespace httc {

template<typename... Args>
void write_fmt(
    std::shared_ptr<uvw::tcp_handle> client, std::format_string<Args...> fmt, Args&&... args
) {
    auto s = std::format(fmt, std::forward<Args>(args)...);
    client->write(s.data(), s.size());
}

Response::Response() : m_status(StatusCode::OK) {
}

void Response::write(sp<uvw::tcp_handle> client) {
    write_fmt(client, "HTTP/1.1 {}\r\n", m_status.code);
    write_fmt(client, "Content-Length: 0\r\n");
    write_fmt(client, "\r\n");
}

void Response::set_status(StatusCode status) {
    m_status = status;
}

Response Response::from_status(StatusCode status) {
    Response r;
    r.m_status = status;
    return r;
}

}
