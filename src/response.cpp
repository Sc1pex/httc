#include "httc/response.h"

namespace httc {

template<typename... Args>
void write_fmt(
    std::shared_ptr<uvw::tcp_handle> client, std::format_string<Args...> fmt, Args&&... args
) {
    auto s = std::format(fmt, std::forward<Args>(args)...);
    client->write(s.data(), s.size());
}

void Response::write(sp<uvw::tcp_handle> client) {
    write_fmt(client, "HTTP/1.1 {}\r\n", m_status);
    write_fmt(client, "\r\n");
}

void Response::set_status(int status) {
    m_status = status;
}

}
