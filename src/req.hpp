#pragma once

#include <optional>
#include <string_view>
#include <uv.h>
#include <vector>

#include "include/fc.hpp"

namespace fc {

struct cookies {
public:
  bool parsed = false;
  std::vector<std::pair<std::string_view, std::string_view>> m_cookies;

  cookies() = default;
  ~cookies() = default;

  void parse(std::string_view header);
  std::optional<std::string_view> get(std::string_view key) const;
};

struct request::impl {
public:
  uv_stream_t *m_remote;
  std::unique_ptr<char[]> m_raw;

  method m_method;
  std::string_view m_path;
  std::string_view m_raw_body;
  std::unique_ptr<cookies> m_cookies = nullptr;
  std::vector<std::pair<std::string_view, std::string_view>> m_params;
  std::vector<std::pair<std::string_view, std::string_view>> m_headers;

  // middlewares + main handler
  std::vector<path_handler> m_handlers;

  impl(uv_stream_t *remote, std::unique_ptr<char[]> raw) : m_remote(remote), m_raw(std::move(raw)) {};
  ~impl() = default;
};

} // namespace fc
