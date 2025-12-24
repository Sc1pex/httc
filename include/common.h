#pragma once

#include <memory>
#include <string_view>

namespace httc {

template<typename T>
using sp = std::shared_ptr<T>;

bool valid_token(std::string_view str);
bool valid_header_value(std::string_view str);

}
