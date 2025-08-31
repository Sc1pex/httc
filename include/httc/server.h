#pragma once

#include <memory>
#include <string>
#include <uvw.hpp>

namespace httc {

template<typename T>
using sp = std::shared_ptr<T>;

class Server {
public:
    Server(sp<uvw::loop> loop);

    void bind_and_listen(const std::string& addr, unsigned int port);

private:
    void handle_conn(uvw::tcp_handle& tcp);

private:
    sp<uvw::loop> m_loop;
};

}
