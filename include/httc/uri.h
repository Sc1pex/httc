#pragma once

#include <optional>
#include <string>
#include <vector>

namespace httc {

enum class URIParseError {};

class URI {
public:
    static std::optional<URI> parse(const std::string& uri);

    const std::vector<std::string>& paths() const;
    const std::vector<std::pair<std::string, std::string>>& query() const;

private:
    std::vector<std::string> m_paths;
    std::vector<std::pair<std::string, std::string>> m_query;
    std::string m_buf;
};

}
