#include <cstring>
#include <string>

#include "http.hpp"
#include "include/fc.hpp"
#include "res.hpp"

namespace fc {

response::~response() {
  delete m_pimpl;
}

response::response(response &&other) noexcept {
  m_pimpl = other.m_pimpl;
  other.m_pimpl = nullptr;
}

response &response::operator=(response &&other) noexcept {
  m_pimpl = other.m_pimpl;
  other.m_pimpl = nullptr;
  return *this;
}

status response::get_status() const {
  return m_pimpl->m_status;
}

response response::ok(status stats) {
  auto res = response(new response::impl(stats, status_to_string(stats)));
  res.set_content_type("text/plain");
  return res;
}

response response::json(nlohmann::json j, status status_) {
  auto res = response(new response::impl(status_, j.dump()));
  res.set_content_type("application/json");
  return res;
}

response response::render(std::string path, status stats) {
  auto res = response(new response::impl(stats, response_file(path, true)));
  res.set_content_type("text/html");
  return res;
}

void response::set_status(status status_) {
  m_pimpl->m_status = status_;
}

void response::set_header(std::string key, std::string value) {
  auto it = std::find_if(m_pimpl->m_headers.begin(), m_pimpl->m_headers.end(), [&](const auto &header) {
    return header.first == key;
  });
  if (it != m_pimpl->m_headers.end()) {
    it->second = value;
  } else {
    m_pimpl->m_headers.push_back(std::make_pair(key, value));
  }
}

void response::set_content_type(std::string content_type) {
  set_header("Content-Type", content_type);
}

} // namespace fc
