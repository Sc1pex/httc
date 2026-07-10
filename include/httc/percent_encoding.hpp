#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace httc {

std::optional<std::string> percent_decode(std::string_view str);
std::string percent_encode(std::string_view str);

}
