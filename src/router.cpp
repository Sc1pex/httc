#include "httc/router.hpp"
#include "httc/request.hpp"
#include "httc/response.hpp"
#include "httc/status.hpp"

namespace httc {

void Router::merge_handler(
    HandlerPath& existing, HandlerFn f, const URI& uri,
    std::optional<std::vector<std::string>> methods
) {
    if (!methods.has_value()) {
        if (existing.global_handler.has_value()) {
            throw URICollision(uri, existing.path);
        }

        existing.global_handler = std::move(f);
        return;
    }

    for (const auto& method : *methods) {
        if (existing.method_handlers.contains(method)) {
            throw URICollision(uri, existing.path);
        }
    }
    for (const auto& method : *methods) {
        existing.method_handlers[method] = f;
    }
}

void Router::add_route(
    HandlerFn f, std::string_view path, std::optional<std::vector<std::string>> methods
) {
    auto uri_opt = URI::parse(path);
    if (!uri_opt.has_value() || !uri_opt->query().empty()) {
        throw InvalidURI(path);
    }
    const URI& uri = *uri_opt;

    for (auto& handler : m_handlers) {
        auto match = handler.path.match(uri);
        if (match == URIMatch::FULL_MATCH) {
            merge_handler(handler, std::move(f), uri, std::move(methods));
            return;
        }
    }

    HandlerPath new_handler(uri);
    if (methods.has_value()) {
        for (const auto& method : *methods) {
            new_handler.method_handlers.emplace(method, f);
        }
    } else {
        new_handler.global_handler = std::move(f);
    }
    m_handlers.push_back(std::move(new_handler));
}

Router& Router::wrap(MiddlewareFn middleware) {
    m_middleware.push_back(std::move(middleware));
    return *this;
}

asio::awaitable<void>
    Router::run_handler(HandlerFn f, const URI& handler_path, Request& req, Response& res) const {
    auto req_paths = req.uri.paths();
    const auto& handler_paths = handler_path.paths();
    for (size_t i = 0; i < handler_paths.size(); i++) {
        if (handler_paths[i] == "*") {
            req.wildcard_path = "";
            for (size_t j = i; j < req_paths.size(); j++) {
                if (j > i) {
                    req.wildcard_path += "/";
                }
                req.wildcard_path += req_paths[j];
            }
            break;
        }

        if (!handler_paths[i].empty() && handler_paths[i][0] == ':') {
            auto param_name = handler_paths[i].substr(1);
            req.path_params[param_name] = req_paths[i];
        }
    }

    const auto& mw_vec = m_middleware;
    size_t middleware_idx = 0;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines)
    auto run_middleware = [&](this const auto& self) -> asio::awaitable<void> {
        if (middleware_idx < mw_vec.size()) {
            const auto& mw = mw_vec[middleware_idx];
            middleware_idx++;
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines)
            co_await mw(req, res, [&](const Request&, Response&) -> asio::awaitable<void> {
                co_await self();
            });
        } else {
            co_await f(req, res);
        }
    };
    co_await run_middleware();
}

asio::awaitable<void> Router::handle(Request& req, Response& res) const {
    // [0] = full match
    // [1] = param match
    // [2] = wildcard match
    std::array<const HandlerPath*, 3> matches = {};

    for (const auto& handler : m_handlers) {
        auto match = handler.path.match(req.uri);
        if (match == URIMatch::FULL_MATCH) {
            matches[0] = &handler;
        } else if (match == URIMatch::PARAM_MATCH) {
            matches[1] = &handler;
        } else if (match == URIMatch::WILD_MATCH) {
            matches[2] = &handler;
        }
    }

    bool method_not_allowed = false;
    for (const auto* const m : matches) {
        if (m != nullptr) {
            if (m->method_handlers.contains(req.method)) {
                co_return co_await run_handler(
                    m->method_handlers.at(req.method), m->path, req, res
                );
            } else if (m->global_handler.has_value()) {
                co_return co_await run_handler(*m->global_handler, m->path, req, res);
            } else if (req.method == "HEAD" && m->method_handlers.contains("GET")) {
                req.method = "GET";
                co_return co_await run_handler(m->method_handlers.at("GET"), m->path, req, res);
            } else if (req.method == "OPTIONS") {
                co_return co_await run_handler(
                    // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines)
                    [&m]([[maybe_unused]] const Request& req, Response& res)
                        -> asio::awaitable<void> {
                        httc::Router::default_options_handler(m, res);
                        co_return;
                    },
                    m->path, req, res
                );
            } else {
                method_not_allowed = true;
            }
        }
    }

    if (method_not_allowed) {
        res.status = StatusCode::METHOD_NOT_ALLOWED;
        co_return;
    }

    res.status = StatusCode::NOT_FOUND;
}

void Router::default_options_handler(const HandlerPath* handler, Response& res) {
    res.status = StatusCode::OK;

    std::string allow;
    // No need to check global_handler, because if it exists
    // it would handle this request instead of calling this function.
    for (const auto& [method, _] : handler->method_handlers) {
        if (!allow.empty()) {
            allow += ", ";
        }
        allow += method;
    }
    if (allow.empty()) {
        allow = "OPTIONS, HEAD";
    } else {
        allow += ", OPTIONS, HEAD";
    }

    res.headers.set("Allow", allow);
}

}
