#pragma once

#include <string_view>

namespace httc {

bool valid_token(std::string_view str);
bool valid_header_value(std::string_view str);
bool valid_cookie_value(std::string_view str);

}
