#include <cassert>
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

std::string_view root_router::norm_path(std::string_view in) {
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
  size_t offset = 1; // skip the leading '/', we assume that the path is normalized
  std::vector<std::string_view> frags;
  while (offset < norm_path.length()) {
    size_t next_slash = norm_path.find('/', offset);
    if (next_slash == std::string_view::npos) {
      frags.emplace_back(norm_path.substr(offset));
      break;
    }
    frags.emplace_back(norm_path.substr(offset, next_slash - offset));
    offset = next_slash + 1;
  }
  return frags;
}

std::string extract_frag_label(frag_type type, const std::string_view &label) {
  if (type == frag_type::DYNAMIC) {
    assert(label.length() > 1 && "Missing dynamic parameter name, try ':id'");
    return std::string(label.substr(1));
  }
  if (type == frag_type::WILDCARD) {
    return std::string(label.substr(1));
  }
  return std::string(label);
}

void root_router::add(method method, const std::string path, path_handler handler, const std::vector<path_handler> &midwares) {
  auto path_fragments = split_path(norm_path(path));
  frag *curr = &m_root;

  for (const auto &frg : path_fragments) {
    frag_type type;

    switch (frg.at(0)) {
    case ':':
      type = frag_type::DYNAMIC;
      break;
    case '*':
      type = frag_type::WILDCARD;
      break;
    default:
      type = frag_type::STATIC;
      break;
    }

    bool found = false;
    frag *prev = nullptr;
    frag *child = curr->m_child;

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
      frag *new_frag = new frag(type, extract_frag_label(type, frg));
      if (prev) {
        prev->m_next = new_frag;
      } else {
        curr->m_child = new_frag;
      }
      child = new_frag;
    }
    curr = child;
  }

  if (!curr->m_handlers) {
    curr->m_handlers = new frag_handlers_t();
  }

  int pos = static_cast<int>(method);
  if (!curr->m_handlers->at(pos).empty()) {
    throw std::runtime_error("Duplicate route for method " + std::to_string(pos) + " at path: " + path);
  }

  curr->m_handlers->at(pos).insert(curr->m_handlers->at(pos).begin(), handler);
  curr->m_handlers->at(pos).insert(curr->m_handlers->at(pos).end(), midwares.begin(), midwares.end());
}

bool root_router::match_and_fill_req(request &req) const {
  auto path = norm_path(req.get_path());
  auto fragments = split_path(path);
  const frag *curr = &m_root;

  if (fragments.empty()) {
    if (curr->m_child && ((curr->m_child->m_type == frag_type::STATIC && curr->m_child->m_label == "/") || curr->m_child->m_type == frag_type::WILDCARD)) {
      req.m_pimpl->m_handlers = curr->m_child->m_handlers->at(static_cast<int>(req.m_pimpl->m_method));
      return true;
    }
    return false;
  }

  for (auto frg : fragments) {
    bool found = false;
    const frag *child = curr->m_child;

    while (child && !found) {
      switch (child->m_type) {
      case frag_type::STATIC:
        found = frg == child->m_label;
        break;
      case frag_type::DYNAMIC:
        found = true;
        req.m_pimpl->m_params.emplace_back(child->m_label, frg);
        break;
      case frag_type::WILDCARD:
        auto handler = child->m_handlers->at(static_cast<int>(req.m_pimpl->m_method));
        assert(!handler.empty());
        req.m_pimpl->m_params.emplace_back(child->m_label, path);
        req.m_pimpl->m_handlers.insert(req.m_pimpl->m_handlers.begin(), handler.begin(), handler.end());
        return true;
      }

      if (found) {
        curr = child;
        break;
      }
      child = child->m_next;
    }

    if (!found) return false;
  }

  if (auto handler = curr->m_handlers->at(static_cast<int>(req.m_pimpl->m_method)); !handler.empty()) {
    req.m_pimpl->m_handlers.insert(req.m_pimpl->m_handlers.begin(), handler.begin(), handler.end());
    return true;
  }
  return false;
}

} // namespace fc
