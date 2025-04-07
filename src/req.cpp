#include <cstddef>
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

std::optional<std::string_view> request::get_header(const std::string &key) const {
  // make key comp. case insesitive
  if (auto it = m_headers.find(key); it != m_headers.end()) return it->second;
  return std::nullopt;
}

std::optional<std::string_view> request::get_cookie(const std::string &name) {
  static auto cookies_header = get_header("Cookie");
  if (!cookies_header.has_value()) {
    return std::nullopt;
  }
  if (!m_cookies.parsed) {
    m_cookies.parsed = true;
    m_cookies.parse(cookies_header.value());
  }
  return m_cookies.get(name);
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

inline std::string_view trim(std::string_view str) {
  while (!str.empty() && std::isspace(static_cast<unsigned char>(str.front())))
    str.remove_prefix(1);
  while (!str.empty() && std::isspace(static_cast<unsigned char>(str.back())))
    str.remove_suffix(1);
  return str;
}

void request::cookies::parse(std::string_view header) {
  while (!header.empty()) {
    size_t semicolon_pos = header.find(';');
    std::string_view token = header.substr(0, semicolon_pos);
    if (semicolon_pos != std::string_view::npos)
      header.remove_prefix(semicolon_pos + 1);
    else
      header = {};
    token = trim(token);
    if (token.empty())
      continue;
    size_t eq_pos = token.find('=');
    if (eq_pos == std::string_view::npos)
      continue;
    std::string_view name = trim(token.substr(0, eq_pos));
    std::string_view value = trim(token.substr(eq_pos + 1));
    if (!name.empty()) {
      m_cookies.push_back({name, value});
    }
  }
}

std::optional<std::string_view> request::cookies::get(std::string_view key) const {
  for (const auto &c : m_cookies) {
    if (c.name == key)
      return c.value;
  }
  return std::nullopt;
}

} // namespace fc
