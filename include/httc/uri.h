#pragma once

#include <optional>
#include <string>
#include <vector>

namespace httc {

// Represents how well a URI pattern matches a given URI.
enum class URIMatch {
    NO_MATCH,
    WILD_MATCH, // e.g., /path/* matches /path/anything/here
    PARAM_MATCH, // e.g., /path/:param matches /path/value, but better than /path/*
    FULL_MATCH, // e.g., /path/exact matches /path/exact, better than anything else
};

class URI {
public:
    URI() = delete;

    static std::optional<URI> parse(const std::string& url_decoded);

    URIMatch match(const URI& other) const;

    const std::vector<std::string>& paths() const;
    const std::vector<std::pair<std::string, std::string>>& query() const;

private:
    URI(std::vector<std::string>&& paths, std::vector<std::pair<std::string, std::string>>&& query)
    : m_paths(std::move(paths)), m_query(std::move(query)) {
    }

private:
    std::vector<std::string> m_paths;
    std::vector<std::pair<std::string, std::string>> m_query;
    std::string m_buf;
};

}
