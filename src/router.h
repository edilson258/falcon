#pragma once

#include "falcon.h"
#include "r3.hpp"

#include <utility>
#include <vector>

namespace fc {

struct route {
  method m_method;
  std::string m_path;
  path_handler m_handler;

  route(const method method_, std::string path, path_handler handler)
      : m_method(method_), m_path(std::move(path)), m_handler(std::move(handler)) {};
};

struct router::impl {
  std::string m_base;
  std::vector<route> m_routes;
  std::vector<path_handler> m_middlewares;

  explicit impl(std::string base) : m_base(std::move(base)) {};
};

struct root_router {
  r3::Tree m_tree;

  root_router() : m_tree(10) {}

  void add(method, const std::string &, const path_handler &, const std::vector<path_handler> &);
  [[nodiscard]] bool match(const request &) const;
};

} // namespace fc
