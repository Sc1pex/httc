#pragma once

#include <filesystem>
#include <string>
#include "httc/request.h"
#include "httc/response.h"

namespace httc {
namespace utils {

class FileServer {
public:
    FileServer(std::string path);

    void operator()(const Request& req, Response& res);

private:
    void handle_dir(std::filesystem::path path, Response& res);
    void handle_file(std::filesystem::path path, Response& res);

private:
    std::string m_path;
};

}
}
