#pragma once

#include <string>
#include <uvw.hpp>
#include "httc/common.h"
#include "httc/router.h"

namespace httc {

class Server {
public:
    Server(sp<uvw::loop> loop, Router router);
    void bind_and_listen(const std::string& addr, unsigned int port);

private:
    void handle_conn(uvw::tcp_handle& tcp);

private:
    sp<Router> m_router;
    sp<uvw::loop> m_loop;
};

}
