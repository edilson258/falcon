#pragma once

#include <string_view>
#include <utility>

#include "include/fc.hpp"

namespace fc {

struct request::cookies {
  bool parsed = false;
  std::vector<std::pair<std::string_view, std::string_view>> m_cookies;
  void parse(std::string_view header);
  std::optional<std::string_view> get(std::string_view key) const;
};

request request_factory(void *remote, std::string_view raw);

} // namespace fc
