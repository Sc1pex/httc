#include "httc/status.h"
#include <optional>

namespace httc {

std::optional<StatusCode> StatusCode::from_int(int c) {
    if (!is_valid(c)) {
        return std::nullopt;
    }

    return StatusCode::create_unchecked(c);
}

bool StatusCode::is_valid(int c) {
    return c >= 100 && c <= 599;
}

StatusCode StatusCode::create_unchecked(int c) {
    StatusCode status;
    status.code = c;
    return status;
}

// 1xx Informational
const StatusCode StatusCode::CONTINUE = StatusCode::create_unchecked(100);
const StatusCode StatusCode::SWITCHING_PROTOCOLS = StatusCode::create_unchecked(101);

// 2xx Success
const StatusCode StatusCode::OK = StatusCode::create_unchecked(200);
const StatusCode StatusCode::CREATED = StatusCode::create_unchecked(201);
const StatusCode StatusCode::ACCEPTED = StatusCode::create_unchecked(202);
const StatusCode StatusCode::NO_CONTENT = StatusCode::create_unchecked(204);
const StatusCode StatusCode::PARTIAL_CONTENT = StatusCode::create_unchecked(206);

// 3xx Redirection
const StatusCode StatusCode::MOVED_PERMANENTLY = StatusCode::create_unchecked(301);
const StatusCode StatusCode::FOUND = StatusCode::create_unchecked(302);
const StatusCode StatusCode::SEE_OTHER = StatusCode::create_unchecked(303);
const StatusCode StatusCode::NOT_MODIFIED = StatusCode::create_unchecked(304);
const StatusCode StatusCode::TEMPORARY_REDIRECT = StatusCode::create_unchecked(307);
const StatusCode StatusCode::PERMANENT_REDIRECT = StatusCode::create_unchecked(308);

// 4xx Client Error
const StatusCode StatusCode::BAD_REQUEST = StatusCode::create_unchecked(400);
const StatusCode StatusCode::UNAUTHORIZED = StatusCode::create_unchecked(401);
const StatusCode StatusCode::FORBIDDEN = StatusCode::create_unchecked(403);
const StatusCode StatusCode::NOT_FOUND = StatusCode::create_unchecked(404);
const StatusCode StatusCode::METHOD_NOT_ALLOWED = StatusCode::create_unchecked(405);
const StatusCode StatusCode::NOT_ACCEPTABLE = StatusCode::create_unchecked(406);
const StatusCode StatusCode::REQUEST_TIMEOUT = StatusCode::create_unchecked(408);
const StatusCode StatusCode::CONFLICT = StatusCode::create_unchecked(409);
const StatusCode StatusCode::GONE = StatusCode::create_unchecked(410);
const StatusCode StatusCode::LENGTH_REQUIRED = StatusCode::create_unchecked(411);
const StatusCode StatusCode::PAYLOAD_TOO_LARGE = StatusCode::create_unchecked(413);
const StatusCode StatusCode::URI_TOO_LONG = StatusCode::create_unchecked(414);
const StatusCode StatusCode::UNSUPPORTED_MEDIA_TYPE = StatusCode::create_unchecked(415);
const StatusCode StatusCode::RANGE_NOT_SATISFIABLE = StatusCode::create_unchecked(416);
const StatusCode StatusCode::EXPECTATION_FAILED = StatusCode::create_unchecked(417);
const StatusCode StatusCode::UNPROCESSABLE_ENTITY = StatusCode::create_unchecked(422);
const StatusCode StatusCode::TOO_MANY_REQUESTS = StatusCode::create_unchecked(429);

// 5xx Server Error
const StatusCode StatusCode::INTERNAL_SERVER_ERROR = StatusCode::create_unchecked(500);
const StatusCode StatusCode::NOT_IMPLEMENTED = StatusCode::create_unchecked(501);
const StatusCode StatusCode::BAD_GATEWAY = StatusCode::create_unchecked(502);
const StatusCode StatusCode::SERVICE_UNAVAILABLE = StatusCode::create_unchecked(503);
const StatusCode StatusCode::GATEWAY_TIMEOUT = StatusCode::create_unchecked(504);
const StatusCode StatusCode::HTTP_VERSION_NOT_SUPPORTED = StatusCode::create_unchecked(505);

}
