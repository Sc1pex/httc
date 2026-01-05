#include "headers.h"
#include "common.h"

namespace httc {

void Headers::set(std::string header, std::string value) {
    if (!valid_token(header)) {
        throw std::invalid_argument("Invalid header name");
    }
    if (!valid_header_value(value)) {
        throw std::invalid_argument("Invalid header value");
    }

    unset(header);

    m_owned_strings.push_back(std::move(header));
    m_owned_strings.push_back(std::move(value));

    std::string_view header_view = m_owned_strings[m_owned_strings.size() - 2];
    std::string_view value_view = m_owned_strings[m_owned_strings.size() - 1];

    m_map.emplace(header_view, value_view);
}

bool Headers::unset(std::string_view header) {
    auto range = m_map.equal_range(header);
    if (range.first != range.second) {
        m_map.erase(range.first, range.second);
        return true;
    } else {
        return false;
    }
}

void Headers::add(std::string header, std::string value) {
    if (!valid_token(header)) {
        throw std::invalid_argument("Invalid header name");
    }
    if (!valid_header_value(value)) {
        throw std::invalid_argument("Invalid header value");
    }

    m_owned_strings.push_back(std::move(header));
    m_owned_strings.push_back(std::move(value));

    std::string_view header_view = m_owned_strings[m_owned_strings.size() - 2];
    std::string_view value_view = m_owned_strings[m_owned_strings.size() - 1];

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
    } else {
        return std::nullopt;
    }
}

}
