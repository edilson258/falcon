#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>

#include "http.hpp"
#include "include/fc.hpp"
#include "templates.hpp"

namespace fc {

const response response::ok(status stats) {
  const char *stats_str = status_to_string(stats);
  return response(std::move(std::format(templates::OK_RESPONSE, static_cast<int>(stats), stats_str, "text/plain", strlen(stats_str), stats_str)));
}

const response response::json(nlohmann::json j, status stats) {
  auto xs = j.dump();
  return response(std::move(std::format(templates::HTTP_RESPONSE_FORMAT, static_cast<int>(stats), status_to_string(stats), "application/json", xs.length(), std::move(xs))));
}

const response response::render(const std::string &filename, status stats) {
  const std::string path = "views/" + filename + ".html";
  if (!std::filesystem::exists(path)) {
    return response::ok(status::INTERNAL_SERVER_ERROR);
  }
  std::ifstream file(path);
  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  const char *stats_str = status_to_string(stats);
  return response(std::move(std::format(templates::OK_RESPONSE, static_cast<int>(stats), stats_str, "text/html", content.length(), content)));
}

void response::set_status(status status) {
  m_status = status;
}

void response::set_content_type(const std::string content_type) {
  m_content_type = content_type;
}

} // namespace fc
