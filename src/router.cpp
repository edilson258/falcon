#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "include/fc.hpp"
#include "req.hpp"
#include "router.hpp"

namespace fc {

router::router(std::string base) : m_pimpl(new impl(std::move(base))) {};

router::~router() { delete m_pimpl; }

void router::get(const std::string path, path_handler handler) {
  m_pimpl->m_routes.push_back(route(method::GET, path, handler));
}

void router::post(const std::string path, path_handler handler) {
  m_pimpl->m_routes.push_back(route(method::POST, path, handler));
}

void router::put(const std::string path, path_handler handler) {
  m_pimpl->m_routes.push_back(route(method::PUT, path, handler));
}

void router::delet(const std::string path, path_handler handler) {
  m_pimpl->m_routes.push_back(route(method::DELETE, path, handler));
}

void router::patch(const std::string path, path_handler handler) {
  m_pimpl->m_routes.push_back(route(method::PATCH, path, handler));
}

void router::use(path_handler middleware) {
  m_pimpl->m_middlewares.insert(m_pimpl->m_middlewares.begin(), middleware);
}

std::string_view root_router::normalize_path(std::string_view in) {
  if (in.empty()) return in;
  char *data = const_cast<char *>(in.data()); // ⚠️ data must be mutable
  size_t ri = 0, wi = 0;
  bool pslash = false;
  while (ri < in.length()) {
    char ch = data[ri++];
    if ((ch == '/' && pslash) || std::isspace(static_cast<unsigned char>(ch))) {
      continue;
    }
    data[wi++] = ch;
    pslash = (ch == '/');
  }
  if (wi > 1 && data[wi - 1] == '/') {
    --wi;
  }
  return std::string_view(data, wi);
}

std::vector<std::string_view> root_router::split_path(const std::string_view norm_path) {
  size_t offset = 1; // skip the leading '/'
  std::vector<std::string_view> parts;
  while (offset < norm_path.length()) {
    size_t next_slash = norm_path.find('/', offset);
    if (next_slash == std::string_view::npos) {
      parts.emplace_back(norm_path.substr(offset));
      break;
    }
    parts.emplace_back(norm_path.substr(offset, next_slash - offset));
    offset = next_slash + 1;
  }
  return parts;
}

void root_router::add(method method, const std::string path, path_handler handler, const std::vector<path_handler> &midwares) {
  auto path_fragments = split_path(normalize_path(path));
  frag *current = &m_root;

  for (const auto &frg : path_fragments) {
    frag_type type;
    switch (frg.at(0)) {
    case ':': type = frag_type::DYNAMIC; break;
    case '*': type = frag_type::WILDCARD; break;
    default: type = frag_type::STATIC; break;
    }

    bool found = false;
    frag *prev = nullptr;
    frag *child = current->m_child;
    while (child) {
      if ((type == frag_type::STATIC && child->m_label == frg) || (type != frag_type::STATIC && child->m_type == type)) {
        if (type == frag_type::DYNAMIC && child->m_label != frg && (child->m_handlers && !child->m_handlers->at((int)method).empty())) {
          throw std::runtime_error("Conflicting dynamic segment names: " + child->m_label + " vs " + std::string(frg));
        }
        found = true;
        break;
      }
      prev = child;
      child = child->m_next;
    }
    if (!found) {
      frag *newFrag = new frag(type, std::string(frag_type::DYNAMIC == type ? frg.substr(1) : frg));
      if (prev) {
        prev->m_next = newFrag;
      } else {
        current->m_child = newFrag;
      }
      child = newFrag;
    }
    current = child;
  }
  if (!current->m_handlers) {
    current->m_handlers = new frag_handlers_t();
  }
  if (!current->m_handlers->at(static_cast<int>(method)).empty()) {
    throw std::runtime_error("Duplicate route for method " + std::to_string(static_cast<int>(method)) + " at path: " + path);
  }
  current->m_handlers->at(static_cast<int>(method)).insert(current->m_handlers->at(static_cast<int>(method)).begin(), handler);
  current->m_handlers->at(static_cast<int>(method)).insert(current->m_handlers->at(static_cast<int>(method)).end(), midwares.begin(), midwares.end());
}

bool root_router::match(request &req) const {
  auto fragments = split_path(normalize_path(req.m_pimpl->m_path));
  const frag *current = &m_root;
  for (auto frg : fragments) {
    bool found = false;
    const frag *child = current->m_child;
    while (child && !found) {
      switch (child->m_type) {
      case frag_type::STATIC: found = frg == child->m_label; break;
      case frag_type::DYNAMIC:
        found = true;
        req.m_pimpl->m_params.emplace_back(child->m_label, frg);
        break;
      case frag_type::WILDCARD:
        // TODOOO: fill handler & midware
        return true;
      }
      if (found) {
        current = child;
        break;
      }
      child = child->m_next;
    }
    if (!found) return false;
  }
  auto handler = current->m_handlers->at(static_cast<int>(req.m_pimpl->m_method));
  if (handler.empty()) return false;
  req.m_pimpl->m_handlers.insert(req.m_pimpl->m_handlers.begin(), handler.begin(), handler.end());
  return true;
}

} // namespace fc
