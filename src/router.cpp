#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "include/fc.hpp"
#include "req.hpp"
#include "router.hpp"
#include "src/debug.hpp"

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

struct route_payload {
public:
  std::string m_path;
  std::vector<path_handler> m_handlers;

  route_payload(std::string path_, std::vector<path_handler> handlers_) : m_path(path_), m_handlers(handlers_) {}
};

void root_router::add(const method method_, const std::string path_, const path_handler handler_, const std::vector<path_handler> middwares_) {
  std::vector<path_handler> handlers = {handler_};
  handlers.insert(handlers.end(), middwares_.begin(), middwares_.end());
  auto payload = new route_payload(path_, handlers);
  if (NULL == m_tree.insert_routel((int)method_, payload->m_path.c_str(), payload->m_path.length(), (void *)payload)) {
    debug::error("Fail to add route %s", payload->m_path.c_str());
  }
}

bool root_router::match(request &req) const {
  r3::MatchEntry entry(req.m_pimpl->m_path.data(), req.m_pimpl->m_path.length());
  entry.set_request_method(static_cast<int>(req.m_pimpl->m_method));
  if (r3::Route matched_route = m_tree.match_route(entry); matched_route) {
    auto payload = reinterpret_cast<route_payload *>(matched_route.data());
    req.m_pimpl->m_handlers.insert(req.m_pimpl->m_handlers.end(), payload->m_handlers.begin(), payload->m_handlers.end());
    return true;
  }
  return false;
}

} // namespace fc
