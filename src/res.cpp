#include <cstring>
#include <string>
#include <utility>

#include "http.hpp"
#include "include/fc.hpp"

namespace fc {

response response::ok(status stats) {
  auto body = status_to_string(stats);
  auto res = response(stats, body);
  res.set_content_type("text/plain");
  return res;
}

response response::json(nlohmann::json j, status stats) {
  auto res = response(stats, j.dump());
  res.set_content_type("application/json");
  return res;
}

response response::render(std::string path, status stats) {
  auto res = response(stats, response::file_info(path, true));
  res.set_content_type("text/html");
  return res;
}

void response::set_status(status status) {
  m_status = status;
}

void response::set_header(std::string key, std::string value) {
  auto it = std::find_if(m_headers.begin(), m_headers.end(), [&](const auto &header) {
    return header.first == key;
  });
  if (it != m_headers.end()) {
    it->second = value;
  } else {
    m_headers.push_back(std::make_pair(key, value));
  }
}

void response::set_content_type(std::string content_type) {
  set_header("Content-Type", content_type);
}

} // namespace fc
