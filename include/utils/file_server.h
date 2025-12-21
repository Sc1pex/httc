#pragma once

#include <filesystem>
#include <string>
#include "../request.h"
#include "../response.h"

namespace httc {
namespace utils {

class FileServer {
public:
    FileServer(std::string path, bool allow_modify = false);

    void operator()(const Request& req, Response& res);
    std::vector<std::string> getAllowedMethods() const;

private:
    void get(const Request& req, Response& res, std::filesystem::path path);
    void put(const Request& req, Response& res, std::filesystem::path path);
    void del(const Request& req, Response& res, std::filesystem::path path);

private:
    bool m_allow_modify;
    std::filesystem::path m_path;
};

}
}
