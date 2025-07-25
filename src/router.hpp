#pragma once

#include "include/fc.hpp"

#include "external/r3/include/r3.hpp"

#include <string_view>
#include <vector>

namespace fc {

struct route {
public:
  method m_method;
  std::string m_path;
  path_handler m_handler;

  route(method method_, std::string path, path_handler handler) : m_method(method_), m_path(std::move(path)), m_handler(handler) {};
};

struct router::impl {
  std::string m_base;
  std::vector<route> m_routes;
  std::vector<path_handler> m_middlewares;

  impl(std::string base) : m_base(std::move(base)) {};
};

struct root_router {
public:
  r3::Tree m_tree;

  root_router() : m_tree(10) {}

  void add(const method, const std::string, const path_handler, const std::vector<path_handler>);
  bool match(request &) const;
};

} // namespace fc
