#include "httc/headers.h"
#include <format>

namespace httc {

void Headers::set(std::string_view header, std::string_view value) {
    auto current = get(header);
    if (current.has_value()) {
        m_map[std::string(header)] = std::format("{}, {}", *current, value);
    } else {
        m_map.emplace(std::string(header), std::string(value));
    }
}

std::optional<std::string_view> Headers::get(std::string_view header) const {
    if (auto it = m_map.find(header); it != m_map.end()) {
        return it->second;
    } else {
        return std::nullopt;
    }
}

}
