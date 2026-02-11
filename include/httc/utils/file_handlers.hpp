#pragma once

#include <asio/awaitable.hpp>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include "httc/request.hpp"
#include "httc/response.hpp"

namespace httc::utils {

class FileHandler {
public:
    explicit FileHandler(std::filesystem::path file_path);

    asio::awaitable<void> operator()(const Request& req, Response& res) const;

    std::vector<std::string> getAllowedMethods() const {
        return { "GET" };
    }

private:
    std::filesystem::path m_file_path;
};

class DirectoryHandler {
public:
    explicit DirectoryHandler(std::filesystem::path base_dir, bool allow_listing = false);

    asio::awaitable<void> operator()(const Request& req, Response& res) const;

    std::vector<std::string> getAllowedMethods() const {
        return { "GET" };
    }

private:
    std::string generate_listing_html(
        const std::string& url_path, const struct DirectoryListing& listing
    ) const;
    std::optional<std::filesystem::path> sanitize_path(std::string_view request_path) const;

private:
    std::filesystem::path m_base_dir;
    bool m_allow_listing;
};

}
