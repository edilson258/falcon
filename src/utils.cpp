#include <iostream>
#include <regex>

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
