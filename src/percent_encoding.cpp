#include "httc/percent_encoding.hpp"
#include <format>

namespace httc {

bool isUnreserved(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-'
           || c == '.' || c == '_' || c == '~';
}

bool isHex(char c) {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

std::optional<std::string> percent_decode(std::string_view str) {
    std::string result;

    for (size_t i = 0; i < str.size(); i++) {
        if (str[i] == '%') {
            if (i + 2 >= str.size() || !isHex(str[i + 1]) || !isHex(str[i + 2])) {
                return std::nullopt;
            }
            auto hex = str.substr(i + 1, 2);
            unsigned char decoded_char = 0;
            auto [ptr, ec] = std::from_chars(hex.begin(), hex.end(), decoded_char, 16);

            if (ec != std::errc{}) {
                return std::nullopt;
            }

            result += static_cast<char>(decoded_char);
            i += 2;
        } else {
            result += str[i];
        }
    }

    return result;
}

std::string percent_encode(std::string_view str) {
    std::string result;

    for (char c : str) {
        if (isUnreserved(c)) {
            result += c;
        } else {
            result += std::format("%{:02X}", static_cast<uint8_t>(c));
        }
    }

    return result;
}

}
