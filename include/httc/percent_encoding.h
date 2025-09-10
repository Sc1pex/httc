#pragma once

#include <optional>
#include <string>

namespace httc {

std::optional<std::string> percent_decode(const std::string& str);
std::string percent_encode(const std::string& str);

}
