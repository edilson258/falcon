#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "falcon.h"
#include "request.h"
#include "router.h"
#include "spdlog/spdlog.h"

namespace fc {

router::router(std::string base) : m_pimpl(new impl(std::move(base))) {};

router::~router() { delete m_pimpl; }

void router::get(const std::string &path, const path_handler &handler) const {
  m_pimpl->m_routes.emplace_back(method::GET, path, handler);
}

void router::post(const std::string &path, const path_handler &handler) const {
  m_pimpl->m_routes.emplace_back(method::POST, path, handler);
}

void router::put(const std::string &path, const path_handler &handler) const {
  m_pimpl->m_routes.emplace_back(method::PUT, path, handler);
}

void router::delet(const std::string &path, const path_handler &handler) const {
  m_pimpl->m_routes.emplace_back(method::DELETE, path, handler);
}

void router::patch(const std::string &path, const path_handler &handler) const {
  m_pimpl->m_routes.push_back(route(method::PATCH, path, handler));
}

void router::use(const path_handler &middleware) const {
  m_pimpl->m_middlewares.insert(m_pimpl->m_middlewares.begin(), middleware);
}

struct route_payload {
  std::string m_path;
  std::vector<path_handler> m_handlers;

  route_payload(const std::string &path_,
                const std::vector<path_handler> &handlers_)
      : m_path(path_), m_handlers(handlers_) {}
};

auto root_router::add(const method method_, const std::string &path_,
                      const path_handler &handler_,
                      const std::vector<path_handler> &middlewares_) -> void {
  std::vector handlers = {handler_};
  handlers.insert(handlers.end(), middlewares_.begin(), middlewares_.end());
  // ReSharper disable once CppDFAMemoryLeak
  const auto payload = new route_payload(path_, handlers);
  if (nullptr ==
      m_tree.insert_routel(static_cast<int>(method_), payload->m_path.c_str(),
                           payload->m_path.length(), (void *)payload)) {
    spdlog::error("Fail to add route {}", payload->m_path.c_str());
  }
}

bool root_router::match(const request &req) const {
  r3::MatchEntry entry(req.m_pimpl->m_path.data(),
                       req.m_pimpl->m_path.length());
  entry.set_request_method(static_cast<int>(req.m_pimpl->m_method));
  const r3::Route matched_route = m_tree.match_route(entry);
  if (!matched_route.is_null()) {
    const auto payload = static_cast<route_payload *>(matched_route.data());
    // std::vector<std::pair<std::string_view, std::string_view>> m_params;
    for (size_t i = 0; i < matched_route.get()->slugs.size; i++) {
      req.m_pimpl->m_params.push_back(
          std::make_pair<std::string_view, std::string_view>(
              std::string_view(entry.get()->vars.slugs.entries[i].base,
                               entry.get()->vars.slugs.entries[i].len),
              std::string_view(entry.get()->vars.tokens.entries[i].base,
                               entry.get()->vars.tokens.entries[i].len)));
    }
    req.m_pimpl->m_handlers.insert(req.m_pimpl->m_handlers.end(),
                                   payload->m_handlers.begin(),
                                   payload->m_handlers.end());
    return true;
  }
  return false;
}

} // namespace fc
