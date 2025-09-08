#include "httc/headers.h"

namespace httc {

void Headers::set(std::string header, std::string value) {
    m_map[std::move(header)] = std::move(value);
}

std::optional<std::string_view> Headers::get(std::string_view header) const {
    if (auto it = m_map.find(header); it != m_map.end()) {
        return it->second;
    } else {
        return std::nullopt;
    }
}

}
