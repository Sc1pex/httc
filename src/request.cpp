#include "httc/request.h"
#include <string_view>

namespace httc {

std::optional<std::string_view> Request::header(std::string_view header) const {
    return m_headers.get(header);
}

std::string_view Request::method() const {
    return m_method;
}

std::string_view Request::uri() const {
    return m_uri;
}

std::string_view Request::body() const {
    return m_body;
}

}
