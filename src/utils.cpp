#include <cassert>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <regex>
#include <tuple>

#include "consts.h"
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

std::filesystem::path join_paths(const std::string &base_str, const std::string &path_str) {
  auto base = std::filesystem::absolute(std::filesystem::path(base_str));
  auto full = base.concat(path_str).lexically_normal().make_preferred();
  return full;
}

char *cstr_from_string(const std::string &str) {
  char *cstr = new char[str.length() + 1];
  std::copy(str.begin(), str.end(), cstr);
  cstr[str.length()] = '\0';
  return cstr;
}

std::string get_content_from_extension(const std::string &ext) {
  auto it = CONTENT_TYPES.find(ext);
  if (it != CONTENT_TYPES.end()) return it->second;
  return "application/octet-stream";
}
