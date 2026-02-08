#include "httc/request.hpp"
#include <string_view>

namespace httc {

Request::Request() : uri(*URI::parse("/")) {
}

}
