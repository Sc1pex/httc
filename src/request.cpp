#include "request.h"
#include <string_view>

namespace httc {

Request::Request() : uri(*URI::parse("/")) {
}

std::optional<std::string_view> Request::header(std::string_view header) const {
    return headers.get(header);
}

}
