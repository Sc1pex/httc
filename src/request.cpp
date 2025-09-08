#include "httc/request.h"

namespace httc {

std::optional<std::string_view> Request::header(const std::string& header) {
    if (m_headers.find(header) != m_headers.end()) {
        return m_headers[header];
    } else {
        return std::nullopt;
    }
}

}
