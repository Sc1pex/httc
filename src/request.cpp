#include "request.h"
#include <string_view>

namespace httc {

Request::Request() : uri(*URI::parse("/")) {
}

}
