#include "httc/router.h"
#include "httc/request.h"
#include "httc/response.h"

namespace httc {

void Router::add_route(
    HandlerFn f, std::string_view path, std::optional<std::vector<std::string>> methods
) {
    auto uri_opt = URI::parse(path);
    if (!uri_opt.has_value() || !uri_opt->query().empty()) {
        throw InvalidURI(path);
    }
    URI uri = *uri_opt;

    for (auto& handlers : m_handlers) {
        auto match = handlers.path.match(uri);
        if (match == URIMatch::FULL_MATCH) {
            if (!methods.has_value()) {
                if (handlers.global_handler.has_value()) {
                    throw URICollision(uri, handlers.path);
                }

                handlers.global_handler = f;

                return;
            }

            for (const auto& method : *methods) {
                if (handlers.method_handlers.contains(method)) {
                    throw URICollision(uri, handlers.path);
                }
            }
            for (const auto& method : *methods) {
                handlers.method_handlers[method] = f;
            }
            return;
        }
    }

    HandlerPath new_handler(uri);
    if (methods.has_value()) {
        for (const auto& method : *methods) {
            new_handler.method_handlers[method] = f;
        }
    } else {
        new_handler.global_handler = f;
    }
    m_handlers.push_back(std::move(new_handler));
}

Response run_handler(HandlerFn f, const URI& handler_path, Request& req) {
    auto req_paths = req.uri.paths();
    auto handler_paths = handler_path.paths();
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
        } else if (!handler_paths[i].empty() && handler_paths[i][0] == ':') {
            auto param_name = handler_paths[i].substr(1);
            req.path_params[param_name] = req_paths[i];
        }
    }

    Response res(req.method == "HEAD");
    f(req, res);
    return res;
}

std::optional<Response> Router::handle(Request& req) const {
    // [0] = full match
    // [1] = param match
    // [2] = wildcard match
    const HandlerPath* matches[3] = {};

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
    for (const auto& m : matches) {
        if (m != nullptr) {
            if (m->method_handlers.contains(req.method)) {
                return run_handler(m->method_handlers.at(req.method), m->path, req);
            } else if (m->global_handler.has_value()) {
                return run_handler(*m->global_handler, m->path, req);
            } else if (req.method == "HEAD" && m->method_handlers.contains("GET")) {
                req.method = "GET";
                return run_handler(m->method_handlers.at("GET"), m->path, req);
            } else {
                method_not_allowed = true;
            }
        }
    }

    if (method_not_allowed) {
        return Response::from_status(StatusCode::METHOD_NOT_ALLOWED);
    }

    return std::nullopt;
}

}
