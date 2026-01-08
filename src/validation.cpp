#include "validation.hpp"

namespace httc {

// https://www.rfc-editor.org/rfc/rfc9110#name-tokens
bool valid_token(std::string_view str) {
    if (str.empty()) {
        return false;
    }
    for (char c : str) {
        if (c == '!' || c == '#' || c == '$' || c == '%' || c == '&' || c == '\'' || c == '*'
            || c == '+' || c == '-' || c == '.' || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')
            || c == '^' || c == '_' || c == '`' || (c >= 'a' && c <= 'z') || c == '|' || c == '~') {
            continue;
        }
        return false;
    }
    return true;
}

bool valid_cookie_value(std::string_view str) {
    for (char c : str) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc == 0x21 || (uc >= 0x23 && uc <= 0x2B) || (uc >= 0x2D && uc <= 0x3A)
            || (uc >= 0x3C && uc <= 0x5B) || (uc >= 0x5D && uc <= 0x7E)) {
            continue;
        }
        return false;
    }
    return true;
}

bool valid_header_value(std::string_view str) {
    for (char c : str) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc == 0x09 || (uc >= 0x20 && uc <= 0x7E) || (uc >= 0x80 && uc <= 0xFF)) {
            continue;
        }
        return false;
    }
    return true;
}

}
