#pragma once

#include <optional>
namespace httc {

struct StatusCode {
public:
    [[nodiscard]] static std::optional<StatusCode> from_int(int code);
    [[nodiscard]] static bool is_valid(int c);

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

public:
    int code;

private:
    static StatusCode create_unchecked(int c);
};

}
