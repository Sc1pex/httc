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
            base += std::format("X-Custom-Header-{}: some-value-{}\r\n", i, i);
        }
    } else if (size == "xl") {
        // ~50KB of headers
        for (int i = 0; i < 1000; ++i) {
            base += std::format("X-Large-Header-{}: {}\r\n", i, std::string(50, 'X'));
        }
    }

    base += "\r\n";
    return base;
}

class StringReader {
public:
    asio::awaitable<std::expected<std::string_view, httc::ReaderError>> pull() {
        if (read) {
            co_return std::unexpected(httc::ReaderError::CLOSED);
        } else {
            read = true;
            co_return str;
        }
    };

    void set_data(std::string data) {
        str = data;
        read = false;
    }

private:
    bool read = false;
    std::string str;
};

asio::awaitable<void> parser_benchmark(std::string_view size, int iterations) {
    std::string request_data = generate_request(size);
    StringReader reader;
    RequestParser parser(1024 * 1024, 16 * 1024 * 1024, reader);

    size_t total_header_len = 0;
    for (int i = 0; i < iterations; ++i) {
        reader.set_data(request_data);
        auto result = co_await parser.next();

        if (result.has_value() && result.value().has_value()) {
            const auto& req = result.value().value();
            for (const auto& [name, value] : req.headers) {
                total_header_len += name.length() + value.length();
            }
        }
    }

    std::cout << total_header_len << std::endl;
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

    asio::io_context io_context;
    asio::co_spawn(io_context, parser_benchmark(size, iterations), asio::detached);
    io_context.run();

    return 0;
}
