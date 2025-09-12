#include "httc/request.h"
#include <string_view>

namespace httc {

Request::Request() : m_uri(*URI::parse("/")) {
}

std::optional<std::string_view> Request::header(std::string_view header) const {
    return m_headers.get(header);
}

std::string_view Request::method() const {
    return m_method;
}

const URI& Request::uri() const {
    return m_uri;
}

std::string_view Request::body() const {
    return m_body;
}

}
