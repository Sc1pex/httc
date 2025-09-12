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

Response::Response() {
    status = StatusCode::OK;
    headers.set("Content-Length", "0");
}

void Response::write(sp<uvw::tcp_handle> client) {
    write_fmt(client, "HTTP/1.1 {}\r\n", status.code);
    for (const auto& [key, value] : headers) {
        write_fmt(client, "{}: {}\r\n", key, value);
    }
    write_fmt(client, "\r\n");
    if (!body.empty()) {
        client->write(body.data(), body.size());
    }
}

Response Response::from_status(StatusCode status) {
    Response r;
    r.status = status;
    return r;
}

}
