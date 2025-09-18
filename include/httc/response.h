#pragma once

#include <uvw/tcp.h>
#include "httc/common.h"
#include "httc/headers.h"
#include "httc/status.h"

namespace httc {

class Response {
public:
    Response(bool is_head_response = false);
    static Response from_status(StatusCode status);

    void write(sp<uvw::tcp_handle> client);

    void set_body(std::string_view body);
    StatusCode status;
    Headers headers;

private:
    std::string m_body;
    bool m_head;
};

}
