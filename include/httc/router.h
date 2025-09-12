#pragma once

#include <functional>
#include <stdexcept>
#include <vector>
#include "httc/request.h"
#include "httc/response.h"
#include "httc/uri.h"

namespace httc {

using HandlerFn = std::function<void(const Request& req, Response& res)>;

struct Handler {
    URI uri;
    HandlerFn h;
    std::optional<const char*> method;
};

class Router {
public:
    Router& route(const char* method, const char* path, HandlerFn handler);
    Router& route(const char* path, HandlerFn handler);

    bool handle(const Request& req, Response& res) const;

private:
    void add_route(Handler h);

private:
    std::vector<Handler> m_handlers;
};

class URICollision : public std::runtime_error {
public:
    URICollision(const URI& uri1, const URI& uri2)
    : std::runtime_error(std::format("URI collision between '{}' and '{}'", uri1, uri2)) {
    }
};

class InvalidURI : public std::runtime_error {
public:
    InvalidURI(const std::string& uri) : std::runtime_error(std::format("Invalid URI: '{}'", uri)) {
    }
};

}
