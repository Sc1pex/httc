#pragma once

#include <asio/awaitable.hpp>
#include <concepts>
#include <functional>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>
#include "httc/request.h"
#include "httc/response.h"
#include "httc/uri.h"

namespace httc {

template<typename T>
concept IsSyncHandler = requires(T t, const Request& req, Response& res) {
    { t(req, res) } -> std::same_as<void>;
};
template<typename T>
concept IsAsyncHandler = requires(T t, const Request& req, Response& res) {
    { t(req, res) } -> std::same_as<asio::awaitable<void>>;
};

template<typename T>
concept IsCallableHandler = IsSyncHandler<T> || IsAsyncHandler<T>;

template<typename T>
concept HasAllowedMethods = requires(T t) {
    { t.getAllowedMethods() } -> std::convertible_to<std::vector<std::string>>;
} && IsCallableHandler<T>;

using HandlerFn = std::function<asio::awaitable<void>(const Request& req, Response& res)>;
using MiddlewareFn =
    std::function<asio::awaitable<void>(Request& req, Response& res, HandlerFn next)>;

template<IsCallableHandler T>
HandlerFn make_handler(T&& handler) {
    if constexpr (IsSyncHandler<std::decay_t<T>>) {
        HandlerFn fn = [handler = std::forward<T>(handler)](
                           const Request& req, Response& res
                       ) mutable -> asio::awaitable<void> {
            co_return handler(req, res);
        };
        return fn;
    } else {
        return std::forward<T>(handler);
    }
}

class Router {
public:
    template<IsCallableHandler T>
    Router& route(std::string_view path, T&& handler) {
        add_route(make_handler(std::forward<T>(handler)), path, std::nullopt);
        return *this;
    }
    template<HasAllowedMethods T>
    Router& route(std::string_view path, T&& handler) {
        auto methods = handler.getAllowedMethods();
        add_route(make_handler(std::forward<T>(handler)), path, methods);
        return *this;
    }

    Router& wrap(MiddlewareFn middleware);

    asio::awaitable<std::optional<Response>> handle(Request& req) const;

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
    Response default_options_handler(const HandlerPath* handler) const;
    asio::awaitable<Response> run_handler(HandlerFn f, const URI& handler_path, Request& req) const;

private:
    std::vector<HandlerPath> m_handlers;
    std::vector<MiddlewareFn> m_middleware;
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
    template<IsCallableHandler T>
    MethodWrapper(T&& f) : m_handler(make_handler(std::forward<T>(f))) {
        m_methods = { std::string(Methods.value)... };
    }

    asio::awaitable<void> operator()(const Request& req, Response& res) {
        co_return co_await m_handler(req, res);
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
