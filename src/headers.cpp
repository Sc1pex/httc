#include "headers.h"
#include <format>
#include "common.h"

namespace httc {

void Headers::set(std::string_view header, std::string_view value) {
    if (!valid_token(header)) {
        throw std::invalid_argument("Invalid header name");
    }
    if (!valid_header_value(value)) {
        throw std::invalid_argument("Invalid header value");
    }

    m_map[std::string(header)] = std::string(value);
}

std::optional<std::string_view> Headers::get(std::string_view header) const {
    if (auto it = m_map.find(header); it != m_map.end()) {
        return it->second;
    } else {
        return std::nullopt;
    }
}

bool Headers::unset(std::string_view header) {
    auto it = m_map.find(header);
    if (it != m_map.end()) {
        m_map.erase(it);
        return true;
    } else {
        return false;
    }
}

void Headers::add(std::string_view header, std::string_view value) {
    auto current = get(header);
    if (current.has_value()) {
        m_map[std::string(header)] = std::format("{}, {}", *current, value);
    } else {
        m_map.emplace(std::string(header), std::string(value));
    }
}

}
