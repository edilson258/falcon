#pragma once

#include <string_view>

#include "include/fc.hpp"

namespace fc {

request request_factory(void *remote, std::string_view raw);

} // namespace fc
