#include <cstdio>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <uv.h>

#include "external/nlohmann/json.hpp"
#include "include/fc.hpp"
#include "req.hpp"

namespace fc {

request::request(request &&other) noexcept {
  this->m_pimpl = other.m_pimpl;
  other.m_pimpl = nullptr;
}

request::~request() { delete m_pimpl; };

response request::next() {
  if (m_pimpl->m_handlers.empty()) {
    throw std::runtime_error("No next function");
  }
  auto next_handler = m_pimpl->m_handlers.back();
  m_pimpl->m_handlers.pop_back();
  return next_handler(*this);
}

std::optional<std::string_view> request::get_param(const std::string &key) {
  if (auto it = std::find_if(m_pimpl->m_params.begin(), m_pimpl->m_params.end(), [key](const std::pair<std::string_view, std::string_view> &p) { return p.first == key; }); it != m_pimpl->m_params.end())
    return it->second;
  return std::nullopt;
}

std::optional<std::string_view> request::get_header(const std::string &key) {
  if (auto it = std::find_if(m_pimpl->m_headers.begin(), m_pimpl->m_headers.end(), [key](const std::pair<std::string_view, std::string_view> &p) { return p.first == key; }); it != m_pimpl->m_headers.end())
    return it->second;
  return std::nullopt;
}

std::optional<std::string_view> request::get_cookie(const std::string &name) {
  static auto cookies_header = get_header("Cookie");
  if (!cookies_header.has_value()) {
    return std::nullopt;
  }
  if (!m_pimpl->m_cookies) {
    m_pimpl->m_cookies = std::make_unique<cookies>();
    m_pimpl->m_cookies->parse(cookies_header.value());
    m_pimpl->m_cookies->parsed = true;
  }
  return m_pimpl->m_cookies->get(name);
}

nlohmann::json request::json() {
  return nlohmann::json::parse(m_pimpl->m_raw_body);
}

inline std::string_view trim(std::string_view str) {
  while (!str.empty() && std::isspace(static_cast<unsigned char>(str.front())))
    str.remove_prefix(1);
  while (!str.empty() && std::isspace(static_cast<unsigned char>(str.back())))
    str.remove_suffix(1);
  return str;
}

void cookies::parse(std::string_view header) {
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

std::optional<std::string_view> cookies::get(std::string_view key) const {
  for (const auto &c : m_cookies) {
    if (c.first == key)
      return c.second;
  }
  return std::nullopt;
}

} // namespace fc
