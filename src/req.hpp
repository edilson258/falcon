#pragma once

#include <string_view>

#include "include/fc.hpp"

namespace fc {

struct request::cookies {
  struct cookie {
    std::string_view name;
    std::string_view value;
  };

  bool parsed = false;
  std::vector<cookie> m_cookies;
  void parse(std::string_view header);
  std::optional<std::string_view> get(std::string_view key) const;
};

request request_factory(void *remote, std::string_view raw);

} // namespace fc
