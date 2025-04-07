#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <sys/stat.h>

#include "external/simdjson/simdjson.h"

namespace fc {

enum class method {
  GET = 0,
  POST,
  PUT,
  DELETE,
  PATCH,

  // used internally to keep track of methods len
  COUNT,
};

enum class status {
  OK = 200,
  CREATED = 201,
  NOT_FOUND = 404,
  INTERNAL_SERVER_ERROR = 500,
};

struct request {
public:
  explicit request() = delete;

  bool bind_to_json(simdjson::dom::element *);
  method get_method() const { return m_method; }
  const void *get_remote() const { return m_remote; }
  const std::string_view &get_raw() const { return m_raw; };
  const std::string_view &get_path() const { return m_path; }
  std::optional<std::string> get_param(const std::string &) const;
  std::optional<std::string_view> get_header(const std::string &) const;
  std::optional<std::string_view> get_cookie(const std::string &);

private:
  void *m_remote;

  method m_method;
  std::string_view m_raw;
  std::string_view m_path;
  std::string_view m_raw_body;
  std::unordered_map<std::string, std::string> m_params;
  std::unordered_map<std::string_view, std::string_view> m_headers;

  request(void *remote, std::string_view raw) : m_remote(remote), m_raw(raw) {};

  struct cookies {
    struct cookie {
      std::string_view name;
      std::string_view value;
    };

    bool parsed = false;
    std::vector<cookie> m_cookies;
    void parse(std::string_view header);
    std::optional<std::string_view> get(std::string_view key) const;
  };

  cookies m_cookies;

  friend struct router;
  friend struct http_parser;
  friend request request_factory(void *remote, std::string_view raw);
};

struct response {
public:
  static const response ok(status stats = status::OK);
  static const response json(std::string body = "{}", status status = status::OK);

  void set_status(status);
  void set_content_type(const std::string);

  const std::string &to_string() const { return m_raw; }

private:
  std::string m_raw;
  status m_status;
  std::string m_content_type;

  response(std::string raw) : m_raw(std::move(raw)) {}
};

using path_handler = std::function<response(request)>;

struct app {
public:
  app();
  ~app();

  void get(const std::string, path_handler);
  void post(const std::string, path_handler);
  void put(const std::string, path_handler);
  void delet(const std::string, path_handler);
  void patch(const std::string, path_handler);

  int listen(const std::string, std::function<void()>);

private:
  struct impl;
  impl *m_pimpl;

  friend void parse_http_request(request);
  friend struct impl;
};
} // namespace fc
