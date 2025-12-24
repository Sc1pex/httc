#include "common.h"

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

}
