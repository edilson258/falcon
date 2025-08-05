#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "falcon.h"
#include "request.h"
#include "router.h"
#include "spdlog/spdlog.h"

namespace fc {
router::router(std::string base_) : m_pimpl(new impl(std::move(base_))) {};

router::~router() { delete m_pimpl; }

router::router(router &&other) noexcept {
  this->m_pimpl = other.m_pimpl;
  other.m_pimpl = nullptr;
};

void router::get(const std::string &path_, const path_handler &handler_) const {
  m_pimpl->m_routes.emplace_back(method::GET, path_, handler_);
}

void router::post(const std::string &path_, const path_handler &handler_) const {
  m_pimpl->m_routes.emplace_back(method::POST, path_, handler_);
}

void router::put(const std::string &path_, const path_handler &handler_) const {
  m_pimpl->m_routes.emplace_back(method::PUT, path_, handler_);
}

void router::delet(const std::string &path_, const path_handler &handler_) const {
  m_pimpl->m_routes.emplace_back(method::DELETE, path_, handler_);
}

void router::patch(const std::string &path, const path_handler &handler_) const {
  m_pimpl->m_routes.emplace_back(method::PATCH, path, handler_);
}

void router::head(const std::string &path_, const path_handler &handler_) const {
  m_pimpl->m_routes.emplace_back(method::HEAD, path_, handler_);
}

void router::options(const std::string &path_, const path_handler &handler_) const {
  m_pimpl->m_routes.emplace_back(method::OPTIONS, path_, handler_);
}

void router::use(const path_handler &middleware) const {
  m_pimpl->m_middlewares.insert(m_pimpl->m_middlewares.begin(), middleware);
}

struct route_payload {
  std::string m_path;
  std::vector<path_handler> m_handlers;

  route_payload(std::string path_, const std::vector<path_handler> &handlers_)
      : m_path(std::move(path_)), m_handlers(handlers_) {}
};

void root_router::add(const method method_, const std::string &path_, const path_handler &handler_,
                      const std::vector<path_handler> &middlewares_) {
  std::vector handlers = {handler_};
  handlers.insert(handlers.end(), middlewares_.begin(), middlewares_.end());
  const auto payload = new route_payload(path_, handlers);
  if (nullptr == m_tree.insert_routel(static_cast<int>(method_), payload->m_path.c_str(),
                                      static_cast<int>(payload->m_path.length()), payload)) {
    spdlog::error("Fail to add route {}", payload->m_path.c_str());
  }
}

bool root_router::match(const request &req) const {
  r3::MatchEntry entry(req.m_pimpl->m_path.data(), static_cast<int>(req.m_pimpl->m_path.length()));
  entry.set_request_method(static_cast<int>(req.m_pimpl->m_method));

  const r3::Route matched_route = m_tree.match_route(entry);
  if (matched_route.is_null()) {
    return false;
  }

  const auto payload = static_cast<route_payload *>(matched_route.data());
  for (size_t i = 0; i < matched_route.get()->slugs.size; i++) {
    auto key = std::string_view(entry.get()->vars.slugs.entries[i].base, entry.get()->vars.slugs.entries[i].len);
    auto value = std::string_view(entry.get()->vars.tokens.entries[i].base, entry.get()->vars.tokens.entries[i].len);
    req.m_pimpl->m_params.emplace_back(key, value);
  }

  req.m_pimpl->m_handlers.insert(req.m_pimpl->m_handlers.end(), payload->m_handlers.begin(), payload->m_handlers.end());
  return true;
}

} // namespace fc
