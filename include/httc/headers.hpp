#pragma once

#include <deque>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace httc {

class Headers {
public:
    // Set a header, replacing any existing entries with the same name.
    void set(std::string header, std::string value);

    // Delete every entry with the given header name.
    // Returns true if any entry was deleted.
    bool unset(std::string_view header);

    // Add a header. If the header already exists, appends the new value to the list of existing
    // values.
    void add(std::string header, std::string value);

    // Set a header, replacing any existing entries with the same name.
    // WARNING: The header and value must be valid for the lifetime of this Headers object.
    void set_view(std::string_view header, std::string_view value);

    // Add a header. If the header already exists, appends the new value to the list of existing
    // values.
    // WARNING: The header and value must be valid for the lifetime of this Headers object.
    void add_view(std::string_view header, std::string_view value);

    // Returns the number of values stored
    [[nodiscard]] std::size_t size() const;

    // Get the first value for the given header, or std::nullopt if not found.
    [[nodiscard]] std::optional<std::string_view> get_one(std::string_view header) const;

    [[nodiscard]] auto get(std::string_view header) const {
        return m_map.equal_range(header);
    }

private:
    // FNV-1a hash for case insensitive strings
    struct CaseInsensitiveHash {
        using is_transparent = void;

        size_t operator()(std::string_view str) const {
            return hash(str);
        }

    private:
        template<typename S>
        size_t hash(const S& str) const {
            const size_t fnv_prime = 10990515241UL;
            size_t hash = 14695981039346656037UL;
            for (char c : str) {
                hash ^= static_cast<unsigned char>(tolower(c));
                hash *= fnv_prime;
            }
            return hash;
        }
    };

    struct CaseInsensitiveSearch {
        using is_transparent = void;

        bool operator()(const std::string& a, const std::string& b) const {
            return compare(a, b);
        }
        bool operator()(const std::string& a, std::string_view b) const {
            return compare(a, b);
        }
        bool operator()(std::string_view a, const std::string& b) const {
            return compare(a, b);
        }
        bool operator()(std::string_view a, std::string_view b) const {
            return compare(a, b);
        }

    private:
        template<typename S1, typename S2>
        bool compare(const S1& a, const S2& b) const {
            if (a.size() != b.size()) {
                return false;
            }
            for (size_t i = 0; i < a.size(); i++) {
                if (tolower(a[i]) != tolower(b[i])) {
                    return false;
                }
            }
            return true;
        }
    };

public:
    // Types for STL container compatibility
    using value_type = std::pair<const std::string_view, std::string_view>;
    using reference = value_type&;
    using const_reference = const value_type&;
    using iterator = std::unordered_multimap<
        std::string_view, std::string_view, CaseInsensitiveHash, CaseInsensitiveSearch>::iterator;
    using const_iterator = std::unordered_multimap<
        std::string_view, std::string_view, CaseInsensitiveHash,
        CaseInsensitiveSearch>::const_iterator;

    // Iterator interface
    iterator begin() {
        return m_map.begin();
    }
    iterator end() {
        return m_map.end();
    }
    const_iterator begin() const {
        return m_map.begin();
    }
    const_iterator end() const {
        return m_map.end();
    }
    const_iterator cbegin() const {
        return m_map.cbegin();
    }
    const_iterator cend() const {
        return m_map.cend();
    }

private:
    std::unordered_multimap<
        std::string_view, std::string_view, CaseInsensitiveHash, CaseInsensitiveSearch>
        m_map;

    std::deque<std::string> m_owned_strings;
    friend struct std::formatter<Headers>;
};

}

template<>
struct std::formatter<httc::Headers> : std::formatter<std::string> {
    auto format(const httc::Headers& headers, std::format_context& ctx) const {
        auto out = ctx.out();
        for (const auto& [key, value] : headers.m_map) {
            out = std::format_to(out, "{}: {}\n", key, value);
        }
        return out;
    }
};
