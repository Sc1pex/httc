#pragma once

#include <uvw/tcp.h>
#include "httc/common.h"
#include "httc/status.h"

namespace httc {

class Response {
public:
    Response();

    void set_status(StatusCode code);

    void write(sp<uvw::tcp_handle> client);

private:
    StatusCode m_status;
};

}
