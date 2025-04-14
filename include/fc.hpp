#pragma once

#include <filesystem>
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
  // 1xx: Informational
  //
  CONTINUE = 100,
  SWITCHING_PROTOCOLS = 101,
  PROCESSING = 102,  // WebDAV
  EARLY_HINTS = 103, // HTTP/1.1 preload

  // 2xx: Success
  //
  OK = 200,
  CREATED = 201,
  ACCEPTED = 202,
  NON_AUTHORITATIVE_INFORMATION = 203,
  NO_CONTENT = 204,
  RESET_CONTENT = 205,
  PARTIAL_CONTENT = 206,
  MULTI_STATUS = 207,     // WebDAV
  ALREADY_REPORTED = 208, // WebDAV
  IM_USED = 226,          // RFC 3229

  // 3xx: Redirection
  //
  MULTIPLE_CHOICES = 300,
  MOVED_PERMANENTLY = 301,
  FOUND = 302,
  SEE_OTHER = 303,
  NOT_MODIFIED = 304,
  USE_PROXY = 305,
  TEMPORARY_REDIRECT = 307,
  PERMANENT_REDIRECT = 308,

  // 4xx: Client Errors
  //
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
  IM_A_TEAPOT = 418, // RFC 2324 / April Fools :P
  MISDIRECTED_REQUEST = 421,
  UNPROCESSABLE_ENTITY = 422, // WebDAV
  LOCKED = 423,               // WebDAV
  FAILED_DEPENDENCY = 424,    // WebDAV
  TOO_EARLY = 425,
  UPGRADE_REQUIRED = 426,
  PRECONDITION_REQUIRED = 428,
  TOO_MANY_REQUESTS = 429,
  REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
  UNAVAILABLE_FOR_LEGAL_REASONS = 451,

  // 5xx: Server Errors
  //
  INTERNAL_SERVER_ERROR = 500,
  NOT_IMPLEMENTED = 501,
  BAD_GATEWAY = 502,
  SERVICE_UNAVAILABLE = 503,
  GATEWAY_TIMEOUT = 504,
  HTTP_VERSION_NOT_SUPPORTED = 505,
  VARIANT_ALSO_NEGOTIATES = 506,
  INSUFFICIENT_STORAGE = 507, // WebDAV
  LOOP_DETECTED = 508,        // WebDAV
  NOT_EXTENDED = 510,
  NETWORK_AUTHENTICATION_REQUIRED = 511
};

struct request;
struct response;

using path_handler = std::function<response(request)>;

struct response {
public:
  static const response ok(status stats = status::OK);
  static const response json(nlohmann::json, status status = status::OK);
  static const response send(const std::filesystem::path path);

  void set_status(status);
  status get_status() const { return m_status; }
  void set_content_type(const std::string);
  const std::string &to_string() const { return m_raw; }

private:
  std::string m_raw;
  status m_status;
  std::string m_content_type;

  response(std::string raw) : m_raw(std::move(raw)) {}
};

struct request {
public:
  explicit request() = delete;
  ~request();

  nlohmann::json json();
  method get_method() const { return m_method; }
  const void *get_remote() const { return m_uvremote; }
  const std::string_view &get_raw() const { return m_raw; };
  const std::string_view &get_path() const { return m_path; }
  std::optional<std::string> get_param(const std::string &) const;
  std::optional<std::string_view> get_header(const std::string &) const;
  std::optional<std::string_view> get_cookie(const std::string &);
  response next();

private:
  const void *m_uvremote;

  method m_method;
  std::string_view m_raw;
  std::string_view m_path;
  std::string_view m_raw_body;
  std::vector<std::pair<std::string_view, std::string>> m_params;
  std::vector<std::pair<std::string_view, std::string_view>> m_headers;
  struct cookies;
  cookies *m_cookies;

  // middlewares + main handler
  std::vector<path_handler> m_handlers;

  request(void *remote, std::string_view raw) : m_uvremote(remote), m_raw(raw) {};

  friend struct root_router;
  friend struct http_parser;
  friend request request_factory(void *remote, std::string_view raw);
};

struct router {
public:
  void get(const std::string, path_handler);
  void post(const std::string, path_handler);
  void put(const std::string, path_handler);
  void delet(const std::string, path_handler);
  void patch(const std::string, path_handler);

  void use(path_handler midware) { m_midwares.insert(m_midwares.begin(), midware); };

  router(std::string base = "") : m_base(base), m_routes() {};

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
  std::vector<path_handler> m_midwares;

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

  void use(const router &);

  int listen(const std::string, std::function<void(const std::string &addr)> = nullptr);

private:
  struct impl;
  impl *m_pimpl;
  friend struct impl;

  friend void parse_http_request(request);
};

} // namespace fc
