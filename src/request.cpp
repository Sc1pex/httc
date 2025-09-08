#include "httc/request.h"

namespace httc {

std::optional<std::string_view> Request::header(const std::string& header) const {
    if (m_headers.find(header) != m_headers.end()) {
        return m_headers.at(header);
    } else {
        return std::nullopt;
    }
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
