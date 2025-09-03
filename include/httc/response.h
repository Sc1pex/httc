#pragma once

#include <uvw/tcp.h>
#include "httc/common.h"

namespace httc {

class Response {
public:
    void set_status(int status);

    void write(sp<uvw::tcp_handle> client);

private:
    int m_status;
};

}
