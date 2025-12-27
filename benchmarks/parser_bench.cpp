#include <httc/request_parser.h>
#include <iostream>
#include <string>

using namespace httc;

std::string generate_request(std::string_view size) {
    std::string base = "GET /api/v1/users/123 HTTP/1.1\r\nHost: api.example.com\r\n";

    if (size == "sm") {
        base += "User-Agent: Benchmark/1.0\r\n"
                "Accept: application/json\r\n"
                "Connection: keep-alive\r\n";
    } else if (size == "lg") {
        for (int i = 0; i < 50; ++i) {
            base += "X-Custom-Header-" + std::to_string(i) + ": some-value-" + std::to_string(i)
                    + "\r\n";
        }
    } else if (size == "xl") {
        // ~50KB of headers
        for (int i = 0; i < 1000; ++i) {
            base += "X-Large-Header-" + std::to_string(i) + ": " + std::string(50, 'X') + "\r\n";
        }
    }

    base += "\r\n";
    return base;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: parser_bench <sm|lg|xl> [iterations]\n";
        return 1;
    }

    std::string size = argv[1];
    int iterations = 100000;
    if (argc > 2) {
        iterations = std::stoi(argv[2]);
    }

    std::string request_data = generate_request(size);
    // Increase buffer size for XL tests
    RequestParser parser(1024 * 1024, 16 * 1024 * 1024);

    size_t total_header_len = 0;
    for (int i = 0; i < iterations; ++i) {
        auto result = parser.feed_data(request_data.data(), request_data.size());

        if (result.has_value() && result.value().has_value()) {
            const auto& req = result.value().value();
            for (const auto& [name, value] : req.headers) {
                total_header_len += name.length() + value.length();
            }
        }
    }

    std::cout << total_header_len << std::endl;
    return 0;
}
