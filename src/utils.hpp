#pragma once

#include <filesystem>
#include <string>

char *cstr_from_string(const std::string &str);
std::tuple<std::string, std::string> split_address(const std::string &);
std::filesystem::path join_paths(const std::string &base_str, const std::string &path_str);
std::string contype_from_ext(const std::string &ext);
