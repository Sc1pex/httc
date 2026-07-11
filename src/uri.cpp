#include "httc/uri.hpp"
#include <optional>
#include <ranges>
#include "httc/percent_encoding.hpp"

namespace httc {

std::optional<std::vector<std::string>> parse_path(std::string_view path) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
    if (path.empty() || path[0] != '/') {
        return std::nullopt;
    }

    auto split = path | std::views::split('/')
                 | std::views::drop(1); // Drop the empty split at the start (path starts with /)

    std::vector<std::string> paths;
    for (auto it = split.begin(); it != split.end(); it++) {
        auto next_it = std::next(it);
        auto is_last = next_it == split.end();

        auto s = *it;

        if (s.empty() && !is_last) {
            continue;
        }

        auto decoded = percent_decode(std::string_view{ s });
        if (!decoded) {
            return std::nullopt;
        }

        if (*decoded == "*" && !is_last) {
            return std::nullopt;
        }
        paths.push_back(std::move(*decoded));
    }
    return paths;
}

std::optional<std::vector<std::pair<std::string, std::string>>>
    parse_query(std::string_view query_sv) {
    auto pairs =
        query_sv | std::views::split('&')
        | std::views::transform([](auto&& q) -> std::pair<std::string_view, std::string_view> {
              auto sv = std::string_view{ q };
              auto eq_pos = sv.find('=');
              if (eq_pos != std::string::npos) {
                  return std::make_pair(sv.substr(0, eq_pos), sv.substr(eq_pos + 1));
              }
              return std::make_pair(sv, "");
          });

    std::vector<std::pair<std::string, std::string>> query;
    for (auto&& [k, v] : pairs) {
        auto dec_k = percent_decode(k);
        auto dec_v = percent_decode(v);
        if (!dec_k || !dec_v) {
            return std::nullopt;
        }
        query.emplace_back(std::move(*dec_k), std::move(*dec_v));
    }

    return query;
}

std::optional<URI> URI::parse(std::string_view url_decoded) {
    auto query_start = url_decoded.find('?');

    auto paths = parse_path(url_decoded.substr(0, query_start));
    if (!paths) {
        return std::nullopt;
    }

    if (query_start == std::string::npos) {
        return URI{ std::move(*paths), {} };
    }

    auto query = parse_query(url_decoded.substr(query_start + 1));
    if (!query) {
        return std::nullopt;
    }

    return URI{ std::move(*paths), std::move(*query) };
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

struct SegmentCompareResult {
    bool is_wildcard = false;
    bool is_param_a = false;
    bool is_param_b = false;
    bool is_mismatch = false;
};

SegmentCompareResult compare_segments(std::string_view a, std::string_view b) {
    if (a == "*" || b == "*") {
        return { .is_wildcard = true };
    }
    if (a == b) {
        return {};
    }
    if (a.empty() || b.empty()) {
        return { .is_mismatch = true };
    }

    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
    bool is_param_a = (a[0] == ':');
    bool is_param_b = (b[0] == ':');
    // NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

    if (!is_param_a && !is_param_b) {
        return { .is_mismatch = true };
    }
    return { .is_param_a = is_param_a, .is_param_b = is_param_b };
}

bool has_trailing_wildcard(auto& longer, auto& shorter) {
    return longer.size() == shorter.size() + 1 && longer.back() == "*";
}

URIMatch URI::match(const URI& other) const {
    std::array<bool, 2> param_match = { false, false };

    for (const auto& [path_a, path_b] : std::views::zip(m_paths, other.m_paths)) {
        auto res = compare_segments(path_a, path_b);
        if (res.is_wildcard) {
            return URIMatch::WILD_MATCH;
        }
        if (res.is_mismatch) {
            return URIMatch::NO_MATCH;
        }
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
        if (res.is_param_a) {
            param_match[0] = true;
        }
        if (res.is_param_b) {
            param_match[1] = true;
        }
        // NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
    }

    if (m_paths.size() != other.m_paths.size()) {
        if (has_trailing_wildcard(m_paths, other.m_paths)
            || has_trailing_wildcard(other.m_paths, m_paths)) {
            return URIMatch::WILD_MATCH;
        }
        return URIMatch::NO_MATCH;
    }
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
    if (param_match[0] && param_match[1]) {
        return URIMatch::FULL_MATCH;
    }
    if (param_match[0] || param_match[1]) {
        return URIMatch::PARAM_MATCH;
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
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
