#include <uvw.hpp>
#include "httc/server.h"

int main() {
    auto loop = uvw::loop::get_default();
    auto server = httc::Server(loop);
    server.bind_and_listen("0.0.0.0", 6969);
}
