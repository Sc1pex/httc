#pragma once

#include <optional>
#include <uvw.hpp>
#include "httc/request.h"

namespace httc {

class RequestParser {
public:
    std::optional<Request> parse_chunk(const char* data, std::size_t length);

private:
    enum class State {
        PARSE_REQUEST_LINE,
        PARSE_HEADERS,
        PARSE_BODY,
    };

    bool feed_line(std::string_view line);

private:
    Request m_req;
    Request m_complete_req;

    std::string m_buffer;
    State m_state;
};

}
