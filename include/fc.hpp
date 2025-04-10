#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <vector>

#include "external/nlohmann/json.hpp"

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
  NO_CONTENT = 204,
  NOT_FOUND = 404,
  INTERNAL_SERVER_ERROR = 500,
};

struct request {
public:
  explicit request() = delete;
  ~request();

  nlohmann::json json();
  method get_method() const { return m_method; }
  const void *get_remote() const { return m_uv_remote; }
  const std::string_view &get_raw() const { return m_raw; };
  const std::string_view &get_path() const { return m_path; }
  std::optional<std::string> get_param(const std::string &) const;
  std::optional<std::string_view> get_header(const std::string &) const;
  std::optional<std::string_view> get_cookie(const std::string &);

private:
  const void *m_uv_remote;

  method m_method;
  std::string_view m_raw;
  std::string_view m_path;
  std::string_view m_raw_body;
  std::vector<std::pair<std::string_view, std::string>> m_params;
  std::vector<std::pair<std::string_view, std::string_view>> m_headers;
  struct cookies;
  cookies *m_cookies; // NOTE: clean on destructor

  request(void *remote, std::string_view raw) : m_uv_remote(remote), m_raw(raw) {};

  friend struct root_router;
  friend struct http_parser;
  friend request request_factory(void *remote, std::string_view raw);
};

struct response {
public:
  static const response ok(status stats = status::OK);
  static const response json(nlohmann::json, status status = status::OK);

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
using middleware_handler = std::function<request(request)>;

struct router {
private:
  struct route {
  public:
    method m_method;
    std::string m_path;
    path_handler m_handler;

    route(method m, std::string path, path_handler handler) : m_method(m), m_path(path), m_handler(handler) {};
  };

  std::string m_base;
  std::vector<route> m_routes;
  middleware_handler m_mh = nullptr;

  friend struct app;

public:
  void get(const std::string, path_handler);
  void post(const std::string, path_handler);
  void put(const std::string, path_handler);
  void delet(const std::string, path_handler);
  void patch(const std::string, path_handler);

  void use(middleware_handler mh) { m_mh = mh; };

  router(std::string base = "") : m_base(base), m_routes() {};
};

struct app {
public:
  app();
  ~app();

  void get(const std::string, path_handler);
  void post(const std::string, path_handler);
  void put(const std::string, path_handler);
  void delet(const std::string, path_handler);
  void patch(const std::string, path_handler);

  void use(const router &);

  int listen(const std::string, std::function<void(const std::string &addr)>);

private:
  struct impl;
  impl *m_pimpl;
  friend struct impl;

  friend void parse_http_request(request);
};

} // namespace fc
