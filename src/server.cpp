#include "httc/server.h"
#include <uvw/stream.h>
#include <memory>
#include <random>

namespace httc {

template<typename... Args>
void write_fmt(
    std::shared_ptr<uvw::tcp_handle> client, std::format_string<Args...> fmt, Args&&... args
) {
    auto s = std::format(fmt, std::forward<Args>(args)...);
    client->write(s.data(), s.size());
}

Server::Server(sp<uvw::loop> loop) : m_loop(loop) {
}

void Server::bind_and_listen(const std::string& addr, unsigned int port) {
    auto tcp = m_loop->resource<uvw::tcp_handle>();

    tcp->on<uvw::listen_event>([this](const uvw::listen_event&, uvw::tcp_handle& tcp) {
        this->handle_conn(tcp);
    });

    tcp->bind(addr, port);
    tcp->listen();
    m_loop->run();
}

void Server::handle_conn(uvw::tcp_handle& tcp) {
    auto client = m_loop->resource<uvw::tcp_handle>();
    tcp.accept(*client);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 100);
    auto target = std::make_shared<int>(dist(gen));

    client->on<uvw::data_event>([client, target](const uvw::data_event& ev, uvw::tcp_handle&) {
        std::string guess_str = std::string(ev.data.get(), ev.length);

        try {
            int guess = std::stoi(guess_str);

            if (guess == *target) {
                write_fmt(client, "Correct! The number was {}\n", *target);
                client->shutdown();
            } else if (guess < *target) {
                write_fmt(client, "Too low! Try again.\n");
            } else {
                write_fmt(client, "Too high! Try again.\n");
            }
        } catch (...) {
            write_fmt(client, "Invalid number\n");
        }
    });

    client->on<uvw::end_event>([](const uvw::end_event&, uvw::tcp_handle& c) {
        c.close();
    });

    write_fmt(client, "Guess the number between 1 and 100\n");
    client->read();
}

}
