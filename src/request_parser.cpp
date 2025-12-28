#include "request_parser.h"

namespace httc {

StatusCode parse_error_to_status_code(RequestParserError error) {
    if (error == RequestParserError::UNSUPPORTED_TRANSFER_ENCODING) {
        return StatusCode::NOT_IMPLEMENTED;
    }
    if (error == RequestParserError::CONTENT_TOO_LARGE) {
        return StatusCode::PAYLOAD_TOO_LARGE;
    }

    return StatusCode::BAD_REQUEST;
}

};
