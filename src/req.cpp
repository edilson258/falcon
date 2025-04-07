#include <exception>
#include <optional>
#include <string>
#include <string_view>

#include "external/simdjson/simdjson.h"
#include "include/fc.hpp"
#include "src/req.hpp"

namespace fc {

request request_factory(void *remote, std::string_view sv) { return request(remote, sv); }

std::optional<std::string> request::get_param(const std::string &key) const {
  if (auto it = m_params.find(key); it != m_params.end()) return it->second;
  return std::nullopt;
}

std::optional<std::string> request::get_header(const std::string &key) const {
  // make key comp. case insesitive
  if (auto it = m_headers.find(key); it != m_headers.end()) return std::string(it->second);
  return std::nullopt;
}

bool request::bind_to_json(simdjson::dom::element *d) {
  static simdjson::dom::parser parser;
  try {
    *d = parser.parse(m_raw_body);
    return true;
  } catch (std::exception &e) {
    std::cerr << "[FALCON ERROR]: Failed to bind request body to json, " << e.what() << std::endl;
    return false;
  }
}

} // namespace fc
