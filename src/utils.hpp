#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

fs::path join_paths(std::string, std::string);
std::tuple<std::string, std::string> split_address(const std::string &);
std::string contype_from_ext(const std::string &ext);
