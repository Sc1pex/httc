#include "httc/router.h"
#include "httc/request.h"

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

bool Router::handle(Request& req, Response& res) const {
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

    for (const auto& m : matches) {
        if (m != nullptr) {
            if (m->method_handlers.contains(req.method)) {
                m->method_handlers.at(req.method)(req, res);
                return true;
            } else if (m->global_handler.has_value()) {
                (*m->global_handler)(req, res);
                return true;
            }
        }
    }

    return false;
}

}
