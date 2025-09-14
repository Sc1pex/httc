#include "httc/utils/file_server.h"
#include <fstream>
#include <print>
#include "httc/status.h"

namespace fs = std::filesystem;

namespace httc {
namespace utils {

FileServer::FileServer(std::string path) : m_path(path) {
}

void FileServer::operator()(const Request& req, Response& res) {
    // Extract the file path from the request URI
    auto file_path = req.uri.path().substr(req.handler_path.size() - 1);

    // Prevent directory traversal attacks
    if (file_path.find("..") != std::string::npos) {
        res.status = StatusCode::BAD_REQUEST;
        res.body = "";
        return;
    }

    // Construct the full file path
    fs::path full_path = fs::path(m_path) / fs::path(file_path);
    if (!fs::exists(full_path)) {
        res.status = StatusCode::NOT_FOUND;
        res.body = "";
        return;
    }

    if (fs::is_directory(full_path)) {
        handle_dir(full_path, res);
    } else if (fs::is_regular_file(full_path)) {
        handle_file(full_path, res);
    } else {
        res.status = StatusCode::NOT_FOUND;
        res.body = "";
    }
}

void FileServer::handle_dir(fs::path path, Response& res) {
    std::string body = "<!DOCTYPE html>\n<html><body><ul>";
    for (const auto& entry : fs::directory_iterator(path)) {
        std::string name = entry.path().filename().string();
        if (fs::is_directory(entry.path())) {
            name += "/";
        }
        body += "<li><a href=\"" + name + "\">" + name + "</a></li>\n";
    }
    body += "</ul></body></html>";
    res.status = StatusCode::OK;
    res.body = body;
    res.headers.override("Content-Length", std::to_string(body.size()));
    res.headers.override("Content-Type", "text/html");
}

void FileServer::handle_file(fs::path path, Response& res) {
    std::ifstream file(path);
    if (!file) {
        res.status = StatusCode::NOT_FOUND;
        res.body = "";
        return;
    }

    std::string body((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    res.status = StatusCode::OK;
    res.body = body;
    res.headers.override("Content-Length", std::to_string(body.size()));
    res.headers.override("Content-Type", "text/plain");
}

}
}
