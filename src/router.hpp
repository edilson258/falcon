#pragma once

#include "include/fc.hpp"

namespace fc {

enum class frag_type {
  STATIC = 1,
  DYNAMIC = 2,
  WILDCARD = 3,
};

struct frag {
public:
  frag_type m_type;
  std::string m_label;
  std::array<path_handler, static_cast<int>(method::COUNT)> m_handlers;

  frag *m_next;
  frag *m_child;

  frag() = default;
  frag(frag_type type, std::string label) : m_type(type), m_label(label), m_next(nullptr), m_child(nullptr) {
    std::fill(m_handlers.begin(), m_handlers.end(), nullptr);
  };
};

struct router {
public:
  frag m_root;

  router() = default;

  void add(method method, const std::string, path_handler);
  path_handler match(method method, const std::string_view &, request &) const;

  static std::vector<std::string> split_path(const std::string_view &);
  static std::string normalize_path(const std::string_view &);
};

} // namespace fc
