#include "httc/uri.h"

namespace httc {

std::optional<URI> URI::parse(const std::string& uri) {
    return std::nullopt;
}

const std::vector<std::string>& URI::paths() const {
    return m_paths;
}

const std::vector<std::pair<std::string, std::string>>& URI::query() const {
    return m_query;
}

}
