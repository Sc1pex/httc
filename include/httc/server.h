#pragma once

#include <string>
#include <uvw.hpp>
#include "common.h"
#include "httc/router.h"

namespace httc {

void bind_and_listen(
    const std::string& addr, unsigned int port, sp<Router> router,
    sp<uvw::loop> loop = uvw::loop::get_default()
);

}
