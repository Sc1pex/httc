#pragma once

#include <string>
#include <uvw.hpp>
#include "httc/common.h"
#include "httc/request.h"
#include "httc/response.h"

namespace httc {

using request_handler_fn = std::function<Response(Request)>;

class Server {
public:
    Server(sp<uvw::loop> loop);

    void set_request_handler(request_handler_fn handler);
    void bind_and_listen(const std::string& addr, unsigned int port);

private:
    void handle_conn(uvw::tcp_handle& tcp);

private:
    sp<uvw::loop> m_loop;
    request_handler_fn m_req_handler;
};

}
