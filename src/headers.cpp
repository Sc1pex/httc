#include "httc/headers.hpp"
#include <cstring>
#include <stdexcept>
#include "httc/validation.hpp"

namespace httc {

Headers::Headers() : m_pool(std::make_unique<std::pmr::monotonic_buffer_resource>()) {
}

std::string_view Headers::allocate_string(std::string_view sv) {
    void* ptr = m_pool->allocate(sv.size());
    std::memcpy(ptr, sv.data(), sv.size());
    return { static_cast<char*>(ptr), sv.size() };
}

void Headers::set(std::string_view header, std::string_view value) {
    if (!valid_token(header)) {
        throw std::invalid_argument("Invalid header name");
    }
    if (!valid_header_value(value)) {
        throw std::invalid_argument("Invalid header value");
    }

    unset(header);

    std::string_view header_view = allocate_string(header);
    std::string_view value_view = allocate_string(value);

    m_map.emplace(header_view, value_view);
}

bool Headers::unset(std::string_view header) {
    auto range = m_map.equal_range(header);
    if (range.first != range.second) {
        m_map.erase(range.first, range.second);
        return true;
    }
    return false;
}

void Headers::add(std::string_view header, std::string_view value) {
    if (!valid_token(header)) {
        throw std::invalid_argument("Invalid header name");
    }
    if (!valid_header_value(value)) {
        throw std::invalid_argument("Invalid header value");
    }

    auto it = m_map.find(header);
    std::string_view header_view;
    if (it != m_map.end()) {
        header_view = it->first;
    } else {
        header_view = allocate_string(header);
    }
    std::string_view value_view = allocate_string(value);

    m_map.emplace(header_view, value_view);
}

void Headers::set_view(std::string_view header, std::string_view value) {
    if (!valid_token(header)) {
        throw std::invalid_argument("Invalid header name");
    }
    if (!valid_header_value(value)) {
        throw std::invalid_argument("Invalid header value");
    }

    unset(header);

    m_map.emplace(header, value);
}

void Headers::add_view(std::string_view header, std::string_view value) {
    if (!valid_token(header)) {
        throw std::invalid_argument("Invalid header name");
    }
    if (!valid_header_value(value)) {
        throw std::invalid_argument("Invalid header value");
    }

    m_map.emplace(header, value);
}

std::size_t Headers::size() const {
    return m_map.size();
}

std::optional<std::string_view> Headers::get_one(std::string_view header) const {
    auto [start, end] = m_map.equal_range(header);
    if (start != end) {
        return start->second;
    }
    return std::nullopt;
}

}
