#include "percent_encoding.h"
#include <format>

namespace httc {

bool isUnreserved(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-'
           || c == '.' || c == '_' || c == '~';
}

bool isHex(char c) {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

std::optional<std::string> percent_decode(const std::string& str) {
    std::string result;

    for (size_t i = 0; i < str.size(); i++) {
        if (str[i] == '%') {
            if (i + 2 >= str.size() || !isHex(str[i + 1]) || !isHex(str[i + 2])) {
                return std::nullopt; // Invalid percent-encoding
            }
            std::string hex = str.substr(i + 1, 2);
            char decoded_char = static_cast<char>(std::stoi(hex, nullptr, 16));
            result += decoded_char;
            i += 2;
        } else {
            result += str[i];
        }
    }

    return result;
}

std::string percent_encode(const std::string& str) {
    std::string result;

    for (unsigned char c : str) {
        if (isUnreserved(c)) {
            result += c;
        } else {
            result += std::format("%{:02X}", static_cast<int>(c));
        }
    }

    return result;
}

}
