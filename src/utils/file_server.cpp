#include "httc/utils/file_server.h"
#include <fstream>
#include <print>
#include "httc/status.h"
#include "httc/utils/mime.h"

namespace fs = std::filesystem;

namespace httc {
namespace utils {

FileServer::FileServer(std::string path, bool allow_modify)
: m_path(path), m_allow_modify(allow_modify) {
}

std::vector<std::string> FileServer::getAllowedMethods() const {
    if (m_allow_modify) {
        return { "GET", "PUT", "DELETE" };
    } else {
        return { "GET" };
    }
}

void FileServer::operator()(const Request& req, Response& res) {
    // Prevent directory traversal attacks
    if (req.wildcard_path.find("..") != std::string::npos) {
        res.status = StatusCode::BAD_REQUEST;
        res.body = "";
        return;
    }

    fs::path full_path = m_path;
    if (!req.wildcard_path.empty()) {
        full_path /= req.wildcard_path;
    }

    if (req.method == "GET") {
        get(req, res, full_path);
    } else if (req.method == "PUT") {
        put(req, res, full_path);
    } else if (req.method == "DELETE") {
        del(req, res, full_path);
    }
}

void serve_file(Response& res, fs::path path) {
    std::ifstream file(path);
    if (!file.is_open() || !file.good()) {
        res.status = StatusCode::INTERNAL_SERVER_ERROR;
        res.body = "Failed to open file";
        res.headers.set("Content-Type", "text/plain");
        res.headers.set("Content-Length", std::to_string(res.body.size()));
        return;
    }

    auto mime_type = utils::mime_type(path).value_or("application/octet-stream");
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    res.status = StatusCode::OK;
    res.body = std::move(content);
    res.headers.set("Content-Type", mime_type);
    res.headers.set("Content-Length", std::to_string(res.body.size()));
}

void serve_directory(Response& res, fs::path path, bool root) {
    std::string body = R"(<!DOCTYPE html>
<html>
<head>
    <title>Directory Listing</title>
    <meta charset="UTF-8">
    <style>
        body { 
            font-family: Arial, sans-serif;
            margin: 40px;
            background: #fafafa;
        }
        h1 { 
            color: #333;
            border-bottom: 1px solid #ddd;
            padding-bottom: 10px;
        }
        a { 
            color: #0066cc;
            text-decoration: none;
        }
        a:hover { 
            text-decoration: underline;
        }
        ul { 
            list-style: none;
            padding: 0;
        }
        li { 
            padding: 5px 0;
        }
        .parent { 
            margin: 10px 0;
            font-weight: bold;
        }
    </style>
</head>
<body>
    <h1>)";

    body += path.string();
    body += "</h1>\n";

    if (!root) {
        body += R"(<div class="parent"><a href="../">.. (parent directory)</a></div>)";
    }

    body += "<ul>\n";

    // Directories first
    for (const auto& entry : fs::directory_iterator(path)) {
        std::string filename = entry.path().filename().string();
        std::string href = filename;

        if (entry.is_directory()) {
            href += "/";
            filename += "/";
            body += "<li><a href=\"" + href + "\">" + filename + "</a></li>\n";
        }
    }

    for (const auto& entry : fs::directory_iterator(path)) {
        std::string filename = entry.path().filename().string();
        std::string href = filename;

        if (!entry.is_directory()) {
            body += "<li><a href=\"" + href + "\">" + filename + "</a></li>\n";
        }
    }

    body += "</ul>\n</body></html>";

    res.status = StatusCode::OK;
    res.body = std::move(body);
    res.headers.set("Content-Type", "text/html");
    res.headers.set("Content-Length", std::to_string(res.body.size()));
}

void FileServer::get(const Request& req, Response& res, fs::path path) {
    if (!fs::exists(path)) {
        res.status = StatusCode::NOT_FOUND;
        res.body = "File not found";
        res.headers.set("Content-Type", "text/plain");
        res.headers.set("Content-Length", std::to_string(res.body.size()));
        return;
    }

    if (fs::is_directory(path)) {
        serve_directory(res, path, path == m_path);
    } else if (fs::is_regular_file(path)) {
        serve_file(res, path);
    } else {
        res.status = StatusCode::FORBIDDEN;
        res.body = "Access denied";
        res.headers.set("Content-Type", "text/plain");
        res.headers.set("Content-Length", std::to_string(res.body.size()));
    }
}

void FileServer::put(const Request& req, Response& res, fs::path path) {
    bool dir = path.has_filename() == false;
    if (dir && fs::exists(path)) {
        res.status = StatusCode::CONFLICT;
        res.body = "Directory already exists";
        res.headers.set("Content-Type", "text/plain");
        res.headers.set("Content-Length", std::to_string(res.body.size()));
        return;
    }

    if (dir) {
        fs::create_directories(path);
        res.status = StatusCode::CREATED;
        return;
    }

    if (fs::exists(path)) {
        // Overwrite existing file
        std::ofstream file(path);
        if (!file.is_open() || !file.good()) {
            res.status = StatusCode::INTERNAL_SERVER_ERROR;
            res.body = "Failed to open file for writing";
            res.headers.set("Content-Type", "text/plain");
            res.headers.set("Content-Length", std::to_string(res.body.size()));
            return;
        }
        file << req.body;
        file.close();
        res.status = StatusCode::OK;
    } else {
        fs::create_directories(path.parent_path());
        std::ofstream file(path);
        if (!file.is_open() || !file.good()) {
            res.status = StatusCode::INTERNAL_SERVER_ERROR;
            res.body = "Failed to create file";
            res.headers.set("Content-Type", "text/plain");
            res.headers.set("Content-Length", std::to_string(res.body.size()));
            return;
        }
        file << req.body;
        file.close();
        res.status = StatusCode::CREATED;
    }
}

void FileServer::del(const Request& req, Response& res, fs::path path) {
    if (!fs::exists(path)) {
        res.status = StatusCode::NOT_FOUND;
        res.body = "File not found";
        res.headers.set("Content-Type", "text/plain");
        res.headers.set("Content-Length", std::to_string(res.body.size()));
        return;
    }

    std::error_code ec;
    if (fs::is_directory(path)) {
        fs::remove_all(path, ec);
    } else {
        fs::remove(path, ec);
    }

    if (ec) {
        res.status = StatusCode::INTERNAL_SERVER_ERROR;
        res.body = "Failed to delete file or directory";
        res.headers.set("Content-Type", "text/plain");
        res.headers.set("Content-Length", std::to_string(res.body.size()));
        return;
    }

    res.status = StatusCode::OK;
}
}
}
