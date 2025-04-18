#include <cstring>
#include <stdexcept>
#include <vector>

#include "include/fc.hpp"
#include "router.hpp"

namespace fc {

void router::get(const std::string path, path_handler handler) {
  m_routes.push_back(route(method::GET, path, handler));
}

void router::post(const std::string path, path_handler handler) {
  m_routes.push_back(route(method::POST, path, handler));
}

void router::put(const std::string path, path_handler handler) {
  m_routes.push_back(route(method::PUT, path, handler));
}

void router::delet(const std::string path, path_handler handler) {
  m_routes.push_back(route(method::DELETE, path, handler));
}

void router::patch(const std::string path, path_handler handler) {
  m_routes.push_back(route(method::PATCH, path, handler));
}

// TODO: normalize in place to avoid std::string allocation
std::string root_router::normalize_path(const std::string_view &input) {
  bool prevSlash = false;
  std::string output;
  output.reserve(input.length());
  for (auto chr : input) {
    if (('/' == chr && prevSlash) || std::isspace(chr)) {
      continue;
    }
    prevSlash = (chr == '/');
    output.push_back(chr);
  }
  return output;
}

// TODO: return vec of string_views to avoid std::string alloc.
std::vector<std::string> root_router::split_path(const std::string_view &path) {
  std::vector<std::string> frags;
  auto normalPath = normalize_path(path);
  char *frag = std::strtok((char *)normalPath.c_str(), "/");
  while (frag) {
    frags.push_back(frag);
    frag = std::strtok(nullptr, "/");
  }
  return frags;
}

void root_router::add(method method, const std::string path, path_handler handler, const std::vector<path_handler> &midwares) {
  auto path_fragments = split_path(path);
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
          throw std::runtime_error("Conflicting dynamic segment names: " + child->m_label + " vs " + frg);
        }
        found = true;
        break;
      }
      prev = child;
      child = child->m_next;
    }
    if (!found) {
      frag *newFrag = new frag(type, frag_type::DYNAMIC == type ? frg.substr(1) : frg);
      if (prev) {
        prev->m_next = newFrag;
      } else {
        current->m_child = newFrag;
      }
      child = newFrag;
    }
    current = child;
  }
  // at this point we know that the current fragment is a leaf node
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
  auto fragments = split_path(req.m_path);
  const frag *current = &m_root;
  for (auto &frg : fragments) {
    bool found = false;
    const frag *child = current->m_child;
    while (child && !found) {
      switch (child->m_type) {
      case frag_type::STATIC: found = frg == child->m_label; break;
      case frag_type::DYNAMIC:
        found = true;
        req.m_params.push_back({child->m_label, frg});
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
    if (!found) {
      return false;
    }
  }
  auto handler = current->m_handlers->at(static_cast<int>(req.m_method));
  if (handler.empty()) {
    return false;
  }
  req.m_handlers.insert(req.m_handlers.begin(), handler.begin(), handler.end());
  return true;
}

} // namespace fc
