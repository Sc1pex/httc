#pragma once

#include <filesystem>
#include <optional>
#include <string_view>

namespace httc {
namespace utils {

std::optional<std::string_view> mime_type(const std::filesystem::path& path);

}
}
