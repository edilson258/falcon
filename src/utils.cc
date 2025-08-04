#include <filesystem>
#include <iostream>
#include <regex>
#include <tuple>

#include "const.h"
#include "falcon.h"
#include "utils.h"

std::tuple<std::string, std::string> split_address(const std::string &input) {
  static const std::regex pattern(R"((.*?):?(\d+))");
  if (std::smatch match; std::regex_match(input, match, pattern)) {
    return {match[1].str().empty() ? "0.0.0.0" : match[1].str(), match[2].str()};
  }
  std::cerr << "[FALCON ERROR]: Provided invalid address, defaulting to 0.0.0.0:8080" << std::endl;
  return {"0.0.0.0", "8080"};
}

fs::path join_paths(const std::string &b, const std::string &p) {
  auto base = fs::absolute(fs::path(b));
  auto full = base.concat(p).lexically_normal().make_preferred();
  return full;
}

std::string content_type_from_ext(const std::string &ext) {
  if (const auto it = CONTENT_TYPES.find(ext); it != CONTENT_TYPES.end())
    return it->second;
  return "application/octet-stream";
}

std::string fc::method_to_string(const method method_) {
  switch (method_) {
  case method::GET:
    return "GET";
  case method::POST:
    return "POST";
  case method::PUT:
    return "PUT";
  case method::PATCH:
    return "PATCH";
  case method::DELETE:
    return "DELETE";
  case method::HEAD:
    return "HEAD";
  case method::OPTIONS:
    return "OPTIONS";
  default:
    return "Unknown Http Method";
  }
}
