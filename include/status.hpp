#pragma once

#include <optional>
#include <string_view>

namespace httc {

struct StatusCode {
public:
    [[nodiscard]] constexpr static bool is_valid(int c) {
        return c >= 100 && c <= 599;
    }
    [[nodiscard]] constexpr static std::optional<StatusCode> from_int(int code) {
        if (!is_valid(code)) {
            return std::nullopt;
        }
        return StatusCode::create_unchecked(code);
    }

    constexpr bool operator==(const StatusCode& other) const = default;

    constexpr std::optional<std::string_view> reason() const {
        switch (code) {
        case 100:
            return "Continue";
        case 101:
            return "Switching Protocols";
        case 200:
            return "OK";
        case 201:
            return "Created";
        case 202:
            return "Accepted";
        case 204:
            return "No Content";
        case 206:
            return "Partial Content";
        case 301:
            return "Moved Permanently";
        case 302:
            return "Found";
        case 303:
            return "See Other";
        case 304:
            return "Not Modified";
        case 307:
            return "Temporary Redirect";
        case 308:
            return "Permanent Redirect";
        case 400:
            return "Bad Request";
        case 401:
            return "Unauthorized";
        case 403:
            return "Forbidden";
        case 404:
            return "Not Found";
        case 405:
            return "Method Not Allowed";
        case 406:
            return "Not Acceptable";
        case 408:
            return "Request Timeout";
        case 409:
            return "Conflict";
        case 410:
            return "Gone";
        case 411:
            return "Length Required";
        case 413:
            return "Payload Too Large";
        case 414:
            return "URI Too Long";
        case 415:
            return "Unsupported Media Type";
        case 416:
            return "Range Not Satisfiable";
        case 417:
            return "Expectation Failed";
        case 422:
            return "Unprocessable Entity";
        case 429:
            return "Too Many Requests";
        case 500:
            return "Internal Server Error";
        case 501:
            return "Not Implemented";
        case 502:
            return "Bad Gateway";
        case 503:
            return "Service Unavailable";
        case 504:
            return "Gateway Timeout";
        case 505:
            return "HTTP Version Not Supported";
        default:
            return std::nullopt;
        }
    }

private:
    constexpr static StatusCode create_unchecked(int c);

public:
    int code;

    // 1xx Informational
    static const StatusCode CONTINUE;
    static const StatusCode SWITCHING_PROTOCOLS;

    // 2xx Success
    static const StatusCode OK;
    static const StatusCode CREATED;
    static const StatusCode ACCEPTED;
    static const StatusCode NO_CONTENT;
    static const StatusCode PARTIAL_CONTENT;

    // 3xx Redirection
    static const StatusCode MOVED_PERMANENTLY;
    static const StatusCode FOUND;
    static const StatusCode SEE_OTHER;
    static const StatusCode NOT_MODIFIED;
    static const StatusCode TEMPORARY_REDIRECT;
    static const StatusCode PERMANENT_REDIRECT;

    // 4xx Client Error
    static const StatusCode BAD_REQUEST;
    static const StatusCode UNAUTHORIZED;
    static const StatusCode FORBIDDEN;
    static const StatusCode NOT_FOUND;
    static const StatusCode METHOD_NOT_ALLOWED;
    static const StatusCode NOT_ACCEPTABLE;
    static const StatusCode REQUEST_TIMEOUT;
    static const StatusCode CONFLICT;
    static const StatusCode GONE;
    static const StatusCode LENGTH_REQUIRED;
    static const StatusCode PAYLOAD_TOO_LARGE;
    static const StatusCode URI_TOO_LONG;
    static const StatusCode UNSUPPORTED_MEDIA_TYPE;
    static const StatusCode RANGE_NOT_SATISFIABLE;
    static const StatusCode EXPECTATION_FAILED;
    static const StatusCode UNPROCESSABLE_ENTITY;
    static const StatusCode TOO_MANY_REQUESTS;

    // 5xx Server Error
    static const StatusCode INTERNAL_SERVER_ERROR;
    static const StatusCode NOT_IMPLEMENTED;
    static const StatusCode BAD_GATEWAY;
    static const StatusCode SERVICE_UNAVAILABLE;
    static const StatusCode GATEWAY_TIMEOUT;
    static const StatusCode HTTP_VERSION_NOT_SUPPORTED;
};

constexpr StatusCode StatusCode::create_unchecked(int c) {
    StatusCode status;
    status.code = c;
    return status;
}

// 1xx Informational
inline constexpr StatusCode StatusCode::CONTINUE = create_unchecked(100);
inline constexpr StatusCode StatusCode::SWITCHING_PROTOCOLS = create_unchecked(101);

// 2xx Success
inline constexpr StatusCode StatusCode::OK = create_unchecked(200);
inline constexpr StatusCode StatusCode::CREATED = create_unchecked(201);
inline constexpr StatusCode StatusCode::ACCEPTED = create_unchecked(202);
inline constexpr StatusCode StatusCode::NO_CONTENT = create_unchecked(204);
inline constexpr StatusCode StatusCode::PARTIAL_CONTENT = create_unchecked(206);

// 3xx Redirection
inline constexpr StatusCode StatusCode::MOVED_PERMANENTLY = create_unchecked(301);
inline constexpr StatusCode StatusCode::FOUND = create_unchecked(302);
inline constexpr StatusCode StatusCode::SEE_OTHER = create_unchecked(303);
inline constexpr StatusCode StatusCode::NOT_MODIFIED = create_unchecked(304);
inline constexpr StatusCode StatusCode::TEMPORARY_REDIRECT = create_unchecked(307);
inline constexpr StatusCode StatusCode::PERMANENT_REDIRECT = create_unchecked(308);

// 4xx Client Error
inline constexpr StatusCode StatusCode::BAD_REQUEST = create_unchecked(400);
inline constexpr StatusCode StatusCode::UNAUTHORIZED = create_unchecked(401);
inline constexpr StatusCode StatusCode::FORBIDDEN = create_unchecked(403);
inline constexpr StatusCode StatusCode::NOT_FOUND = create_unchecked(404);
inline constexpr StatusCode StatusCode::METHOD_NOT_ALLOWED = create_unchecked(405);
inline constexpr StatusCode StatusCode::NOT_ACCEPTABLE = create_unchecked(406);
inline constexpr StatusCode StatusCode::REQUEST_TIMEOUT = create_unchecked(408);
inline constexpr StatusCode StatusCode::CONFLICT = create_unchecked(409);
inline constexpr StatusCode StatusCode::GONE = create_unchecked(410);
inline constexpr StatusCode StatusCode::LENGTH_REQUIRED = create_unchecked(411);
inline constexpr StatusCode StatusCode::PAYLOAD_TOO_LARGE = create_unchecked(413);
inline constexpr StatusCode StatusCode::URI_TOO_LONG = create_unchecked(414);
inline constexpr StatusCode StatusCode::UNSUPPORTED_MEDIA_TYPE = create_unchecked(415);
inline constexpr StatusCode StatusCode::RANGE_NOT_SATISFIABLE = create_unchecked(416);
inline constexpr StatusCode StatusCode::EXPECTATION_FAILED = create_unchecked(417);
inline constexpr StatusCode StatusCode::UNPROCESSABLE_ENTITY = create_unchecked(422);
inline constexpr StatusCode StatusCode::TOO_MANY_REQUESTS = create_unchecked(429);

// 5xx Server Error
inline constexpr StatusCode StatusCode::INTERNAL_SERVER_ERROR = create_unchecked(500);
inline constexpr StatusCode StatusCode::NOT_IMPLEMENTED = create_unchecked(501);
inline constexpr StatusCode StatusCode::BAD_GATEWAY = create_unchecked(502);
inline constexpr StatusCode StatusCode::SERVICE_UNAVAILABLE = create_unchecked(503);
inline constexpr StatusCode StatusCode::GATEWAY_TIMEOUT = create_unchecked(504);
inline constexpr StatusCode StatusCode::HTTP_VERSION_NOT_SUPPORTED = create_unchecked(505);

}
