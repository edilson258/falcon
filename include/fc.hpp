#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <sys/stat.h>

#include "external/nlohmann/json.hpp"

namespace fc {

enum class method {
  GET = 2,
  POST = 2 << 1,
  PUT = 2 << 2,
  DELETE = 2 << 3,
  PATCH = 2 << 4,
  HEAD = 2 << 5,
  OPTIONS = 2 << 6,
};

enum class status {
  CONTINUE = 100,
  SWITCHING_PROTOCOLS = 101,
  PROCESSING = 102,
  EARLY_HINTS = 103,
  OK = 200,
  CREATED = 201,
  ACCEPTED = 202,
  NON_AUTHORITATIVE_INFORMATION = 203,
  NO_CONTENT = 204,
  RESET_CONTENT = 205,
  PARTIAL_CONTENT = 206,
  MULTI_STATUS = 207,
  ALREADY_REPORTED = 208,
  IM_USED = 226,
  MULTIPLE_CHOICES = 300,
  MOVED_PERMANENTLY = 301,
  FOUND = 302,
  SEE_OTHER = 303,
  NOT_MODIFIED = 304,
  USE_PROXY = 305,
  TEMPORARY_REDIRECT = 307,
  PERMANENT_REDIRECT = 308,
  BAD_REQUEST = 400,
  UNAUTHORIZED = 401,
  PAYMENT_REQUIRED = 402,
  FORBIDDEN = 403,
  NOT_FOUND = 404,
  METHOD_NOT_ALLOWED = 405,
  NOT_ACCEPTABLE = 406,
  PROXY_AUTHENTICATION_REQUIRED = 407,
  REQUEST_TIMEOUT = 408,
  CONFLICT = 409,
  GONE = 410,
  LENGTH_REQUIRED = 411,
  PRECONDITION_FAILED = 412,
  PAYLOAD_TOO_LARGE = 413,
  URI_TOO_LONG = 414,
  UNSUPPORTED_MEDIA_TYPE = 415,
  RANGE_NOT_SATISFIABLE = 416,
  EXPECTATION_FAILED = 417,
  IM_A_TEAPOT = 418,
  MISDIRECTED_REQUEST = 421,
  UNPROCESSABLE_ENTITY = 422,
  LOCKED = 423,
  FAILED_DEPENDENCY = 424,
  TOO_EARLY = 425,
  UPGRADE_REQUIRED = 426,
  PRECONDITION_REQUIRED = 428,
  TOO_MANY_REQUESTS = 429,
  REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
  UNAVAILABLE_FOR_LEGAL_REASONS = 451,
  INTERNAL_SERVER_ERROR = 500,
  NOT_IMPLEMENTED = 501,
  BAD_GATEWAY = 502,
  SERVICE_UNAVAILABLE = 503,
  GATEWAY_TIMEOUT = 504,
  HTTP_VERSION_NOT_SUPPORTED = 505,
  VARIANT_ALSO_NEGOTIATES = 506,
  INSUFFICIENT_STORAGE = 507,
  LOOP_DETECTED = 508,
  NOT_EXTENDED = 510,
  NETWORK_AUTHENTICATION_REQUIRED = 511
};

struct response {
public:
  static response ok(status stats = status::OK);
  static response json(nlohmann::json, status stats = status::OK);
  static response render(std::string path, status stats = status::OK);

  void set_status(status);
  status get_status() const;
  void set_content_type(std::string);
  void set_header(std::string, std::string);

  ~response();
  response(const response &other) = delete;
  response &operator=(const response &other) = delete;
  response(response &&other) noexcept;
  response &operator=(response &&other) = delete;

private:
  struct impl;
  impl *m_pimpl;

  response(struct impl *pimpl) : m_pimpl(pimpl) {};

  friend struct app;
};

struct request {
public:
  response next();

  nlohmann::json json();
  method get_method();
  std::string_view &get_path();

  std::optional<std::string_view> get_param(const std::string &);
  std::optional<std::string_view> get_header(const std::string &);
  std::optional<std::string_view> get_cookie(const std::string &);

  ~request();
  request(const request &other) = delete;
  request &operator=(const request &other) = delete;
  request(request &&other) noexcept;
  request &operator=(request &&other) = delete;

private:
  struct impl;
  impl *m_pimpl;

  request(struct impl *impl) : m_pimpl(impl) {}

  friend struct app;
  friend struct http_parser;
  friend struct root_router;
};

using path_handler = std::function<response(request &)>;

struct router {
public:
  void get(const std::string, path_handler);
  void post(const std::string, path_handler);
  void put(const std::string, path_handler);
  void delet(const std::string, path_handler);
  void patch(const std::string, path_handler);

  router(std::string base);

  void use(path_handler middleware);

  ~router();
  router(const router &other) = delete;
  router &operator=(const router &other) = delete;
  router(router &&other) noexcept;
  router &operator=(router &&other) = delete;

private:
  struct impl;
  impl *m_pimpl;

  friend struct app;
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

  void set_views_dir(const std::string);
  void set_assets_dir(const std::string);

  void use(const router &);

  int listen(const std::string, std::function<void(const std::string &addr)> = nullptr);

private:
  struct impl;
  impl *m_pimpl;
};

std::string method_to_string(method);

} // namespace fc
