#pragma once

#include <string_view>

#include "include/fc.hpp"

namespace fc {

enum class frag_type {
  STATIC = 1,
  DYNAMIC = 2,
  WILDCARD = 3,
};

using frag_handlers_t = std::array<path_handler, static_cast<int>(method::COUNT)>;

struct frag {
public:
  frag_type m_type;
  std::string m_label;
  middleware_handler m_middleware;
  frag_handlers_t *m_handlers;

  frag *m_next;
  frag *m_child;

  frag() = default;
  frag(frag_type type, std::string label, middleware_handler mh = nullptr) : m_type(type), m_label(label), m_middleware(mh), m_handlers(nullptr), m_next(nullptr), m_child(nullptr) {};
};

struct root_router {
public:
  frag m_root;

  root_router() = default;

  void add(method method, const std::string, path_handler, middleware_handler mh = nullptr);
  path_handler match(request &) const;

  static std::string normalize_path(const std::string_view &);
  static std::vector<std::string> split_path(const std::string_view &);
};

} // namespace fc
