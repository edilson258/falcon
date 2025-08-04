#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

fs::path join_paths(const std::string &, const std::string &);
std::tuple<std::string, std::string> split_address(const std::string &);
std::string content_type_from_ext(const std::string &ext);
