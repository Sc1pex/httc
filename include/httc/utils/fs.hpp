#pragma once

#include <asio/awaitable.hpp>
#include <expected>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>
#include "httc/response.hpp"

namespace httc::utils {

struct DirectoryListing {
    std::vector<std::string> entries;
    std::size_t files_start_index;
};

std::expected<DirectoryListing, std::error_code> list_directory(const std::filesystem::path& path);

asio::awaitable<void> serve_file(const std::filesystem::path& path, Response& res);

}
