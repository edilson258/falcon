#pragma once

#include <string_view>
#include <vector>

#include "include/fc.hpp"

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

enum class frag_type {
  STATIC = 1,
  DYNAMIC = 2,
  WILDCARD = 3,
};

using frag_handlers_t = std::array<std::vector<path_handler>, static_cast<int>(method::PATCH)>;

struct frag {
public:
  frag_type m_type;
  std::string m_label;
  frag_handlers_t *m_handlers;

  frag *m_next;
  frag *m_child;

  frag() : m_handlers(nullptr) {};
  frag(frag_type type, std::string label) : m_type(type), m_label(label), m_handlers(nullptr), m_next(nullptr), m_child(nullptr) {};
};

struct root_router {
public:
  frag m_root;

  root_router() = default;

  void add(method, const std::string, path_handler, const std::vector<path_handler> &);
  bool match_and_fill_req(request &) const;

  static std::string_view norm_path(std::string_view);
  static std::vector<std::string_view> split_path(const std::string_view);
};

} // namespace fc
