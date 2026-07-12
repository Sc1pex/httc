#include "httc/validation.hpp"
#include <algorithm>

namespace httc {

// https://www.rfc-editor.org/rfc/rfc9110#name-tokens
bool valid_token(std::string_view str) {
    if (str.empty()) {
        return false;
    }
    return std::ranges::all_of(str, [](auto c) {
        return (
            c == '!' || c == '#' || c == '$' || c == '%' || c == '&' || c == '\'' || c == '*'
            || c == '+' || c == '-' || c == '.' || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')
            || c == '^' || c == '_' || c == '`' || (c >= 'a' && c <= 'z') || c == '|' || c == '~'
        );
    });
}

bool valid_cookie_value(std::string_view str) {
    return std::ranges::all_of(str, [](auto c) {
        return (
            c == 0x21 || (c >= 0x23 && c <= 0x2B) || (c >= 0x2D && c <= 0x3A)
            || (c >= 0x3C && c <= 0x5B) || (c >= 0x5D && c <= 0x7E)
        );
    });
}

bool valid_header_value(std::string_view str) {
    return std::ranges::all_of(str, [](auto c) {
        auto uc = static_cast<unsigned char>(c);
        return (uc == 0x09 || (uc >= 0x20 && uc <= 0x7E) || uc >= 0x80);
    });
}

}
