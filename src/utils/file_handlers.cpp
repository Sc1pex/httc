#include "httc/utils/file_handlers.hpp"
#include <format>
#include "httc/utils/fs.hpp"

namespace httc::utils {

FileHandler::FileHandler(std::filesystem::path file_path) : m_file_path(std::move(file_path)) {
}

asio::awaitable<void> FileHandler::operator()(const Request&, Response& res) const {
    co_await serve_file(m_file_path, res);
}

DirectoryHandler::DirectoryHandler(std::filesystem::path base_dir, bool allow_listing)
: m_base_dir(base_dir), m_allow_listing(allow_listing) {
}

asio::awaitable<void> DirectoryHandler::operator()(const Request& req, Response& res) const {
    auto path_opt = sanitize_path(req.wildcard_path);
    if (!path_opt) {
        res.status = StatusCode::FORBIDDEN;
        co_return;
    }

    std::filesystem::path full_path = *path_opt;
    std::error_code ec;

    if (std::filesystem::is_directory(full_path, ec)) {
        std::string url_path = req.uri.path();
        if (url_path.empty() || url_path.back() != '/') {
            res.status = StatusCode::MOVED_PERMANENTLY;
            res.headers.set("Location", url_path + "/");
            co_return;
        }

        auto index_path = full_path / "index.html";
        if (std::filesystem::exists(index_path, ec)) {
            co_await serve_file(index_path, res);
            co_return;
        }

        if (m_allow_listing) {
            auto listing_res = list_directory(full_path);
            if (!listing_res) {
                res.status = StatusCode::INTERNAL_SERVER_ERROR;
                co_return;
            }

            std::string html = generate_listing_html(req.wildcard_path, *listing_res);
            res.status = StatusCode::OK;
            res.headers.set("Content-Type", "text/html");
            res.set_body(std::move(html));
        } else {
            res.status = StatusCode::FORBIDDEN;
        }
    } else {
        co_await serve_file(full_path, res);
    }
}

std::optional<std::filesystem::path>
    DirectoryHandler::sanitize_path(std::string_view request_path) const {
    std::filesystem::path rel_path(request_path);

    if (rel_path.is_absolute()) {
        return std::nullopt;
    }
    for (const auto& part : rel_path) {
        if (part == "..") {
            return std::nullopt;
        }
    }

    return m_base_dir / rel_path;
}

std::string DirectoryHandler::generate_listing_html(
    const std::string& url_path, const DirectoryListing& listing
) const {
    const auto index_of = m_base_dir.string() + "/" + url_path;
    std::string html = std::format("<html><head><title>Index of {}</title></head><body>", index_of);
    html += std::format("<h1>Index of {}</h1><hr><ul>", index_of);

    if (url_path != "/" && !url_path.empty()) {
        html += "<li><a href=\"..\">..</a></li>";
    }

    for (size_t i = 0; i < listing.entries.size(); ++i) {
        const auto& name = listing.entries[i];
        bool is_dir = i < listing.files_start_index;

        std::string display_name = name + (is_dir ? "/" : "");
        std::string href = name + (is_dir ? "/" : "");
        html += std::format("<li><a href=\"{}\">{}</a></li>", href, display_name);
    }
    html += "</ul><hr></body></html>";
    return html;
}

}
