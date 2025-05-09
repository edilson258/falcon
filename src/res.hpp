#pragma once

#include "include/fc.hpp"

namespace fc {

struct response_file {
  std::string m_path;
  bool m_isview;

  response_file() = default;
  response_file(std::string path, bool is_view = false) : m_path(std::move(path)), m_isview(is_view) {};
};

struct response::impl {
  status m_status;
  std::string m_body;
  std::vector<std::pair<std::string, std::string>> m_headers;

  // on send file as response
  bool m_isfile;
  response_file m_file;

  impl(status status_, response_file file) : m_status(status_), m_isfile(true), m_file(std::move(file)) {}
  impl(status status_, std::string body) : m_status(status_), m_body(std::move(body)), m_isfile(false) {}
};

} // namespace fc
