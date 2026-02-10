#include "httc/utils/fs.hpp"
#include <algorithm>
#include <asio/redirect_error.hpp>
#include <asio/stream_file.hpp>
#include <asio/this_coro.hpp>
#include <asio/use_awaitable.hpp>
#include "httc/utils/mime.hpp"

namespace httc::utils {

std::expected<DirectoryListing, std::error_code> list_directory(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        return std::unexpected(
            ec ? ec : std::make_error_code(std::errc::no_such_file_or_directory)
        );
    }
    if (!std::filesystem::is_directory(path, ec)) {
        return std::unexpected(std::make_error_code(std::errc::not_a_directory));
    }

    std::vector<std::string> dirs;
    std::vector<std::string> files;

    for (const auto& entry : std::filesystem::directory_iterator(path, ec)) {
        if (entry.is_directory()) {
            dirs.push_back(entry.path().filename().string());
        } else if (entry.is_regular_file()) {
            files.push_back(entry.path().filename().string());
        }
    }

    if (ec) {
        return std::unexpected(ec);
    }

    std::sort(dirs.begin(), dirs.end());
    std::sort(files.begin(), files.end());

    DirectoryListing listing;
    listing.files_start_index = dirs.size();
    listing.entries = std::move(dirs);
    listing.entries.insert(
        listing.entries.end(), std::make_move_iterator(files.begin()),
        std::make_move_iterator(files.end())
    );

    return listing;
}

asio::awaitable<void> serve_file(const std::filesystem::path& path, Response& res) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || !std::filesystem::is_regular_file(path, ec)) {
        res.status = StatusCode::NOT_FOUND;
        co_return;
    }

    auto size = std::filesystem::file_size(path, ec);
    if (ec) {
        res.status = StatusCode::INTERNAL_SERVER_ERROR;
        co_return;
    }

    auto mime = mime_type(path);
    if (mime) {
        res.headers.set("Content-Type", std::string(*mime));
    } else {
        res.headers.set("Content-Type", "application/octet-stream");
    }

    auto executor = co_await asio::this_coro::executor;
    asio::stream_file file(executor);

    file.open(path.string(), asio::stream_file::read_only, ec);
    if (ec) {
        res.status = StatusCode::FORBIDDEN;
        co_return;
    }

    auto stream = co_await res.send_fixed(size);

    char buffer[8192];
    while (true) {
        std::size_t n = co_await file.async_read_some(
            asio::buffer(buffer), asio::redirect_error(asio::use_awaitable, ec)
        );

        if (ec == asio::error::eof || n == 0) {
            break;
        } else if (ec) {
            co_return;
        }

        co_await stream.write(std::string_view(buffer, n));
    }

    co_return;
}

}
