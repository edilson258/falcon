#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <tuple>

char *cstr_from_string(const std::string &str);
std::tuple<std::string, std::string> split_address(const std::string &);
std::optional<std::filesystem::path> validate_and_resolve_path(std::string base, std::string path);

std::string get_content_from_extension(const std::string &ext);
