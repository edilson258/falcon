#include <algorithm>
#include <string>

#include "falcon.h"
#include "http.h"
#include "response.h"

namespace fc {

response::~response() {
  delete m_pimpl;
}

response::response(response &&other) noexcept {
  m_pimpl = other.m_pimpl;
  other.m_pimpl = nullptr;
}

status response::get_status() const {
  return m_pimpl->m_status;
}

response response::ok(status stats) {
  auto res = response(new response::impl(stats, status_to_string(stats)));
  res.set_content_type("text/plain");
  return res;
}

response response::json(nlohmann::json j, status stats) {
  auto res = response(new impl(stats, j.dump()));
  res.set_content_type("application/json");
  return res;
}

response response::render(std::string path, status stats) {
  auto res = response(new response::impl(stats, response_file(path, true)));
  res.set_content_type("text/html");
  return res;
}

void response::set_status(const status status_) const {
  m_pimpl->m_status = status_;
}

void response::set_header(std::string key, const std::string &value) {
  const auto it = std::ranges::find_if(m_pimpl->m_headers, [&](const auto &header) {
    return header.first == key;
  });
  if (it != m_pimpl->m_headers.end()) {
    it->second = value;
  } else {
    m_pimpl->m_headers.emplace_back(key, value);
  }
}

void response::set_content_type(const std::string &content_type) {
  set_header("Content-Type", content_type);
}

} // namespace fc
