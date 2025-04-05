#pragma once

#include <string_view>

#include "fc.hpp"

namespace fc
{

Req RequestFactory(void *remote, std::string_view sv);

} // namespace fc
