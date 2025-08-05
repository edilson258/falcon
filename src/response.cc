#include <algorithm>
#include <string>

#include "falcon.h"
#include "http.h"
#include "response.h"

namespace fc {

response::~response() { delete m_pimpl; }

response::response(response &&other) noexcept {
  m_pimpl = other.m_pimpl;
  other.m_pimpl = nullptr;
}

status response::get_status() const { return m_pimpl->m_status; }

response response::ok(const status status_) { return response(new impl(status_, status_to_string(status_))); }

response response::json(const nlohmann::json &json_, const status status_) {
  head_t content_type{"Content-Type", "application/json"};
  return response(new impl(status_, json_.dump(), {content_type}));
}

response response::render(std::string path_, const status status_) {
  head_t content_type{"Content-Type", "text/html"};
  auto file = response_file{std::move(path_), file_loader::Render};
  return response(new impl(status_, std::move(file), {content_type}));
}

void response::set_status(const status status_) const { m_pimpl->m_status = status_; }

void response::set_header(std::string key, const std::string &value) {
  const auto it = std::ranges::find_if(m_pimpl->m_headers, [&](const auto &header) { return header.first == key; });
  if (it != m_pimpl->m_headers.end()) {
    it->second = value;
  } else {
    m_pimpl->m_headers.emplace_back(key, value);
  }
}

void response::set_content_type(const std::string &content_type) { set_header("Content-Type", content_type); }

} // namespace fc
