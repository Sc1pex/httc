#include "httc/utils/fs.hpp"
#include <algorithm>
#include <asio/post.hpp>
#include <asio/this_coro.hpp>
#include <asio/use_awaitable.hpp>
#include <fstream>
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

    std::ifstream file{ path, std::ios::binary };
    if (!file) {
        res.status = StatusCode::FORBIDDEN;
        co_return;
    }

    constexpr std::size_t buffer_size = 8192;
    std::size_t bytes_remaining = size;
    std::array<char, buffer_size> read_buf;

    auto stream = co_await res.send_fixed(size);

    while (bytes_remaining > 0) {
        std::size_t to_read = std::min(buffer_size, bytes_remaining);

        auto read_result = co_await asio::async_initiate<
            decltype(asio::use_awaitable), void(std::optional<std::string_view>)>(
            [&file, to_read, &read_buf, pool_ex = res.thread_pool_executor()](auto handler) {
                asio::post(
                    pool_ex, [handler = std::move(handler), &file, to_read, &read_buf]() mutable {
                        file.read(read_buf.data(), static_cast<std::streamsize>(to_read));
                        auto bytes_read = static_cast<size_t>(file.gcount());

                        std::optional<std::string_view> result = std::nullopt;
                        if (bytes_read > 0) {
                            result = std::string_view{ read_buf.data(), bytes_read };
                        }

                        asio::post(
                            asio::get_associated_executor(handler),
                            [handler = std::move(handler), result = std::move(result)]() mutable {
                                handler(std::move(result));
                            }
                        );
                    }
                );
            },
            asio::use_awaitable
        );

        if (!read_result) {
            // Read error occurred
            co_return;
        }
        co_await stream.write(*read_result);
        bytes_remaining -= read_result->size();
    }

    co_return;
}

}
