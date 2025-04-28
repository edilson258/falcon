#include <cstring>
#include <filesystem>
#include <string>
#include <utility>

#include "http.hpp"
#include "include/fc.hpp"

namespace fc {

response response::ok(status stats) {
  auto body = status_to_string(stats);
  auto res = response(stats, body, false);
  res.set_content_type("text/plain");
  res.set_header("Content-Len", std::to_string(strlen(body)));
  return res;
}

response response::json(nlohmann::json j, status stats) {
  auto body = j.dump();
  auto res = response(stats, body, false);
  res.set_content_type("application/json");
  res.set_header("Content-Len", std::to_string(body.length()));
  return res;
}

response response::render(std::string filename, status stats) {
  // TODO: allow users to specify a custom path
  std::string path = "views/" + filename + ".html";
  if (!std::filesystem::exists(path)) {
    return response::ok(status::INTERNAL_SERVER_ERROR);
  }
  auto res = response(stats, std::move(path), true);
  res.set_content_type("text/html");
  res.set_header("Transfer-Encoding", "chunked");
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
