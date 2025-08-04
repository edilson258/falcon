#pragma once

#include "falcon.h"
#include "utils.h"

#include <filesystem>
#include <variant>

namespace fs = std::filesystem;

namespace fc {

using head_t = std::pair<std::string, std::string>;
using heads_t = std::vector<std::pair<std::string, std::string>>;

using response_text = std::string;

enum class file_loader {
  Render,
  External,
};

struct response_file {
  std::string m_path;
  const file_loader m_loader;

  response_file(std::string path_, const file_loader loader_)
      : m_path(std::move(path_)), m_loader(loader_) {}

  explicit response_file(const response &other) = delete;
  response_file &operator=(const response_file &other) = delete;
  response_file &operator=(response_file &&other) = delete;
  ~response_file() = default;
  response_file(response_file &&other) noexcept
      : m_path(std::move(other.m_path)), m_loader(other.m_loader) {}
};

struct response::impl {
  status m_status;
  heads_t m_headers{};

  std::variant<response_text, response_file> m_payload;

  impl(const status status_, response_text text_, const heads_t &headers_ = {}) {
    m_status = status_;
    m_payload.emplace<response_text>(std::move(text_));

    for (const auto &[key, value] : headers_) {
      set_header(key, value);
    }

    try_set_header("Content-Type", "text/plain");
    try_set_header("Content-Len", std::to_string(text_.length()));
  };

  impl(const status status_, response_file file_, const heads_t &headers_ = {}) {
    m_status = status_;
    m_payload.emplace<response_file>(std::move(file_));

    for (const auto &[key, value] : headers_) {
      set_header(key, value);
    }

    const auto file_ext = fs::path(file_.m_path).extension().string();
    try_set_header("Content-Type", content_type_from_ext(file_ext));
  }

  [[nodiscard]] bool is_text() const { return m_payload.index() == 0; }
  [[nodiscard]] bool is_file() const { return m_payload.index() == 1; }

  void set_header(const std::string &key, const std::string &value) {
    const auto existing = find_header(key);
    if (existing.has_value()) {
      existing.value()->second = value;
    } else {
      m_headers.emplace_back(key, value);
    }
  }

  void try_set_header(const std::string &key, const std::string &value) {
    const auto existing = find_header(key);
    if (existing.has_value()) {
      return;
    }
    m_headers.emplace_back(key, value);
  }

  std::optional<head_t *> find_header(const std::string &key) {
    const auto it = std::ranges::find_if(
        m_headers, [&](const auto &header) { return header.first == key; });
    return it == m_headers.end() ? std::nullopt : std::optional{&*it};
  }
};

} // namespace fc
