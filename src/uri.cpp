#include "uri.h"
#include <optional>
#include <print>
#include <ranges>
#include "percent_encoding.h"

namespace httc {

std::optional<URI> URI::parse(std::string_view uri) {
    auto query_start = uri.find("?");
    auto path = std::string(uri.substr(0, query_start));

    std::vector<std::string> paths;
    std::vector<std::pair<std::string, std::string>> query;

    if (path.empty() || path[0] != '/') {
        return std::nullopt;
    }

    size_t start = 1;
    size_t end = path.find("/", start);
    while (end != std::string::npos) {
        if (end > start) {
            auto str = path.substr(start, end - start);
            if (str == "*") {
                if (end != path.size() - 1) {
                    return std::nullopt;
                }
            }
            paths.push_back(path.substr(start, end - start));
        }
        start = end + 1;
        end = path.find("/", start);
    }
    paths.push_back(path.substr(start));

    // Percent decode each path segment
    for (auto& p : paths) {
        auto decoded = percent_decode(p);
        if (!decoded.has_value()) {
            return std::nullopt;
        }
        p = *decoded;
    }

    if (query_start == std::string::npos) {
        return URI{ std::move(paths), std::move(query) };
    }

    std::string query_str = std::string(uri.substr(query_start + 1));
    start = 0;
    end = query_str.find("&", start);
    while (end != std::string::npos) {
        auto eq_pos = query_str.find("=", start);
        if (eq_pos != std::string::npos && eq_pos < end) {
            query.emplace_back(
                query_str.substr(start, eq_pos - start),
                query_str.substr(eq_pos + 1, end - eq_pos - 1)
            );
        } else {
            query.emplace_back(query_str.substr(start, end - start), "");
        }
        start = end + 1;
        end = query_str.find("&", start);
    }
    if (start < query_str.size()) {
        auto eq_pos = query_str.find("=", start);
        if (eq_pos != std::string::npos) {
            query.emplace_back(
                query_str.substr(start, eq_pos - start), query_str.substr(eq_pos + 1)
            );
        } else {
            query.emplace_back(query_str.substr(start), "");
        }
    }

    // Percent decode each query key and value
    for (auto& [key, value] : query) {
        auto decoded_key = percent_decode(key);
        if (!decoded_key.has_value()) {
            return std::nullopt;
        }
        key = *decoded_key;

        auto decoded_value = percent_decode(value);
        if (!decoded_value.has_value()) {
            return std::nullopt;
        }
        value = *decoded_value;
    }

    return URI{ std::move(paths), std::move(query) };
}

const std::vector<std::string>& URI::paths() const {
    return m_paths;
}

const std::vector<std::pair<std::string, std::string>>& URI::query() const {
    return m_query;
}

std::optional<std::string_view> URI::query_param(std::string_view param) const {
    for (const auto& [key, value] : m_query) {
        if (key == param) {
            return value;
        }
    }
    return std::nullopt;
}

URIMatch URI::match(const URI& other) const {
    bool param_match[2] = { false, false };

    for (const auto& [path_a, path_b] : std::views::zip(m_paths, other.m_paths)) {
        if (path_a == "*" || path_b == "*") {
            return URIMatch::WILD_MATCH;
        }
        if (path_a != path_b) {
            if (path_a.length() == 0 || path_b.length() == 0) {
                return URIMatch::NO_MATCH;
            }

            if (path_a[0] == ':') {
                param_match[0] = true;
            }
            if (path_b[0] == ':') {
                param_match[1] = true;
            }

            if (path_a[0] != ':' && path_b[0] != ':') {
                return URIMatch::NO_MATCH;
            }
        }
    }

    if (m_paths.size() != other.m_paths.size()) {
        if (m_paths.size() == other.m_paths.size() + 1 && m_paths.back() == "*") {
            return URIMatch::WILD_MATCH;
        }
        if (other.m_paths.size() == m_paths.size() + 1 && other.m_paths.back() == "*") {
            return URIMatch::WILD_MATCH;
        }
        return URIMatch::NO_MATCH;
    }
    if (param_match[0] && param_match[1]) {
        return URIMatch::FULL_MATCH;
    }
    if (param_match[0] || param_match[1]) {
        return URIMatch::PARAM_MATCH;
    }
    return URIMatch::FULL_MATCH;
}

std::string URI::to_string() const {
    return std::format("{}", *this);
}

std::string URI::path() const {
    std::string result;
    for (const auto& p : m_paths) {
        result += "/" + p;
    }
    return result.empty() ? "/" : result;
}

}
