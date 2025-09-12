#include "httc/router.h"
#include "httc/request.h"

namespace httc {

Router& Router::route(const char* method, const char* path, HandlerFn handler) {
    auto uri = URI::parse(path);
    if (!uri.has_value()) {
        throw InvalidURI(path);
    }
    add_route({ *uri, handler, method });
    return *this;
}

Router& Router::route(const char* path, HandlerFn handler) {
    auto uri = URI::parse(path);
    if (!uri.has_value()) {
        throw InvalidURI(path);
    }
    add_route({ *uri, handler, std::nullopt });
    return *this;
}

void Router::add_route(Handler h) {
    for (const auto& handlers : m_handlers) {
        auto match = handlers.uri.match(h.uri);
        if (h.method == handlers.method && match == URIMatch::FULL_MATCH) {
            throw URICollision(h.uri, handlers.uri);
        }
    }
    m_handlers.push_back(std::move(h));
}

bool Router::handle(const Request& req, Response& res) const {
    // [0] = full match
    // [1] = param match
    // [2] = wildcard match
    std::optional<HandlerFn> matches[3];

    for (const auto& handler : m_handlers) {
        if (handler.method.has_value() && req.method != *handler.method) {
            continue;
        }
        auto match = handler.uri.match(req.uri);
        if (match == URIMatch::FULL_MATCH) {
            matches[0] = handler.h;
            break;
        } else if (match == URIMatch::PARAM_MATCH) {
            matches[1] = handler.h;
        } else if (match == URIMatch::WILD_MATCH) {
            matches[2] = handler.h;
        }
    }

    for (const auto& m : matches) {
        if (m.has_value()) {
            (*m)(req, res);
            return true;
        }
    }

    return false;
}

}
