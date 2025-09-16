#pragma once

#include <concepts>
#include <functional>
#include <stdexcept>
#include <string_view>
#include <vector>
#include "httc/request.h"
#include "httc/response.h"
#include "httc/uri.h"

namespace httc {

using HandlerFn = std::function<void(const Request& req, Response& res)>;

template<typename T>
concept IsCallableHandler = requires(T t, const Request& req, Response& res) { t(req, res); };

template<typename T>
concept HasAllowedMethods = requires(T t) {
    { t.getAllowedMethods() } -> std::convertible_to<std::vector<std::string>>;
} && IsCallableHandler<T>;

class Router {
public:
    template<IsCallableHandler T>
    Router& route(std::string_view path, T handler) {
        add_route(handler, path, std::nullopt);
        return *this;
    }
    template<HasAllowedMethods T>
    Router& route(std::string_view path, T handler) {
        add_route(handler, path, handler.getAllowedMethods());
        return *this;
    }

    bool handle(Request& req, Response& res) const;

private:
    struct HandlerPath {
        HandlerPath(URI p) : path(p) {
        }

        URI path;
        std::unordered_map<std::string, HandlerFn> method_handlers;
        std::optional<HandlerFn> global_handler;
    };

    void add_route(
        HandlerFn f, std::string_view path, std::optional<std::vector<std::string>> methods
    );

private:
    std::vector<HandlerPath> m_handlers;
};

// Helper to convert string literals to char arrays
template<size_t N>
struct StringLiteral {
    constexpr StringLiteral(const char (&str)[N]) {
        std::copy_n(str, N, value);
    }
    char value[N];
};

template<StringLiteral... Methods>
class MethodWrapper {
public:
    MethodWrapper(HandlerFn f) : m_handler(f) {
        m_methods = { std::string(Methods.value)... };
    }

    void operator()(const Request& req, Response& res) {
        m_handler(req, res);
    }

    std::vector<std::string> getAllowedMethods() const {
        return m_methods;
    }

private:
    std::vector<std::string> m_methods;
    HandlerFn m_handler;
};

namespace methods {
using get = MethodWrapper<"GET">;
using post = MethodWrapper<"POST">;
using put = MethodWrapper<"PUT">;
using del = MethodWrapper<"DELETE">;
using patch = MethodWrapper<"PATCH">;
using head = MethodWrapper<"HEAD">;
using options = MethodWrapper<"OPTIONS">;
}

class URICollision : public std::runtime_error {
public:
    URICollision(const URI& uri1, const URI& uri2)
    : std::runtime_error(std::format("URI collision between '{}' and '{}'", uri1, uri2)) {
    }
};

class InvalidURI : public std::runtime_error {
public:
    InvalidURI(std::string_view uri) : std::runtime_error(std::format("Invalid URI: '{}'", uri)) {
    }
};

}
