#include "httc/request_parser.h"
#include <print>
#include <ranges>

namespace httc {

std::optional<Request> RequestParser::parse_chunk(const char* data, std::size_t length) {
    std::string s(data, length);
    m_buffer += s;

    auto delim = std::string_view("\r\n");
    for (const auto line : std::views::split(m_buffer, delim)) {
        auto sv = std::string_view(line);
        bool full_req = feed_line(sv);
        if (full_req) {
            return m_complete_req;
        }
    }

    return std::nullopt;
}

bool RequestParser::feed_line(std::string_view line) {
    if (m_state == State::PARSE_REQUEST_LINE) {
        // Split the request line by space
        auto parts = std::views::split(line, ' ');
        auto it = parts.begin();

        // First part is the method
        if (it != parts.end()) {
            m_req.m_method = std::string((*it).begin(), (*it).end());
            ++it;
        }

        // Second part is the path
        if (it != parts.end()) {
            m_req.m_path = std::string((*it).begin(), (*it).end());
            ++it;
        }

        // Third part is the version
        if (it != parts.end()) {
            m_req.m_version = std::string((*it).begin(), (*it).end());
            ++it;
        }

        m_state = State::PARSE_HEADERS;
    } else if (m_state == State::PARSE_HEADERS) {
        if (line.empty()) {
            // End of headers
            m_state = State::PARSE_BODY;
        } else {
            // Split the header line by ": "
            auto parts = std::views::split(line, std::string_view(": "));
            auto it = parts.begin();

            if (it != parts.end()) {
                std::string key((*it).begin(), (*it).end());
                ++it;
                if (it != parts.end()) {
                    std::string value((*it).begin(), (*it).end());
                    m_req.m_headers[key] = value;
                }
            }
        }
    } else {
        if (line.empty()) {
            // End of body
            m_state = State::PARSE_REQUEST_LINE;
            m_complete_req = m_req;
            m_req = Request(); // Reset for next request

            return true;
        } else {
            m_req.m_body += std::string(line.begin(), line.end()) + "\r\n";
        }
    }

    return false;
}

}
