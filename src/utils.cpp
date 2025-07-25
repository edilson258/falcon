#include <cassert>
#include <filesystem>
#include <iostream>
#include <regex>
#include <termios.h>
#include <tuple>

#include "const.hpp"
#include "include/fc.hpp"
#include "utils.hpp"

std::tuple<std::string, std::string> split_address(const std::string &input) {
  std::smatch match;
  static const std::regex pattern(R"((.*?):?(\d+))");
  if (std::regex_match(input, match, pattern)) {
    return {match[1].str().empty() ? "0.0.0.0" : match[1].str(), match[2].str()};
  }
  std::cerr << "[FALCON ERROR]: Provided invalid address, defaulting to 0.0.0.0:8080" << std::endl;
  return {"0.0.0.0", "8080"};
}

fs::path join_paths(std::string b, std::string p) {
  auto base = fs::absolute(fs::path(b));
  auto full = base.concat(p).lexically_normal().make_preferred();
  return full;
}

std::string contype_from_ext(const std::string &ext) {
  auto it = CONTENT_TYPES.find(ext);
  if (it != CONTENT_TYPES.end()) return it->second;
  return "application/octet-stream";
}

std::string fc::method_to_string(fc::method method_) {
  switch (method_) {
  case fc::method::GET: return "GET";
  case fc::method::POST: return "POST";
  case fc::method::PUT: return "PUT";
  case fc::method::PATCH: return "PATCH";
  case fc::method::DELETE: return "DELETE";
  default: return "Unknown Http Method";
  }
}
