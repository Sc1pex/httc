#include "httc/utils/fs.hpp"
#include <algorithm>
#include <asio/post.hpp>
#include <asio/this_coro.hpp>
#include <asio/use_awaitable.hpp>
#include <fstream>
#include <memory>
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

    auto stream = co_await res.send_fixed(size);

    // Use thread pool executor for blocking file I/O
    auto thread_pool_executor = res.thread_pool_executor();

    auto file = std::make_shared<std::ifstream>(path, std::ios::binary);
    if (!*file) {
        res.status = StatusCode::FORBIDDEN;
        co_return;
    }

    constexpr std::size_t buffer_size = 8192;
    std::size_t bytes_remaining = size;

    while (bytes_remaining > 0) {
        std::size_t to_read = std::min(buffer_size, bytes_remaining);

        auto read_result = co_await asio::async_initiate<
            decltype(asio::use_awaitable), void(std::optional<std::string>)>(
            [](auto handler, asio::any_io_executor file_executor,
               std::shared_ptr<std::ifstream> file_ptr, std::size_t to_read) {
                // Post the blocking file I/O to the thread pool
                asio::post(
                    file_executor, [handler = std::move(handler), file_ptr, to_read]() mutable {
                        std::string buffer(to_read, '\0');
                        file_ptr->read(buffer.data(), to_read);
                        auto bytes_read = file_ptr->gcount();

                        if (bytes_read <= 0) {
                            asio::post(
                                asio::get_associated_executor(handler),
                                [handler = std::move(handler)]() mutable {
                                    handler(std::nullopt);
                                }
                            );
                            return;
                        }

                        buffer.resize(bytes_read);
                        // Post the result back to the event loop
                        asio::post(
                            asio::get_associated_executor(handler),
                            [handler = std::move(handler), buffer = std::move(buffer)]() mutable {
                                handler(std::move(buffer));
                            }
                        );
                    }
                );
            },
            asio::use_awaitable, thread_pool_executor, file, to_read
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
