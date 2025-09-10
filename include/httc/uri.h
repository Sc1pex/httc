#pragma once

#include <optional>
#include <string>
#include <vector>

namespace httc {

enum class URIParseError {};

class URI {
public:
    URI() = delete;

    static std::optional<URI> parse(const std::string& url_decoded);

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
