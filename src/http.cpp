#include <cstdio>
#include <cstring>
#include <string_view>

#include "http.hpp"

namespace fc {

enum llhttp_errno http_parser::parse(request *req) {
  m_llhttp_instance.data = req;
  enum llhttp_errno err = llhttp_execute(&m_llhttp_instance, req->m_raw.data(), req->m_raw.length());
  llhttp_reset(&m_llhttp_instance);
  return err;
}

int http_parser::llhttp_on_url(llhttp_t *p, const char *at, size_t len) {
  request *r = (request *)p->data;
  r->m_path = std::string_view(at, len);
  return HPE_OK;
}

int http_parser::llhttp_on_method(llhttp_t *p, const char *at, size_t len) {
  request *r = (request *)p->data;
  if (strncmp("GET", at, len) == 0)
    r->m_method = method::GET;
  else if (strncmp("POST", at, len) == 0)
    r->m_method = method::POST;
  else if (strncmp("PUT", at, len) == 0)
    r->m_method = method::PUT;
  else if (strncmp("DELETE", at, len) == 0)
    r->m_method = method::DELETE;
  else
    return HPE_INVALID_METHOD;
  return HPE_OK;
}

int http_parser::llhttp_on_body(llhttp_t *p, const char *at, size_t len) {
  request *req = (request *)p->data;
  req->m_raw_body = std::string_view(at, len);
  return HPE_OK;
}

int http_parser::llhttp_on_header_field(llhttp_t *p, const char *at, size_t len) {
  request *req = (request *)p->data;
  req->m_headers.push_back({std::string_view(at, len), {}});
  return HPE_OK;
}

int http_parser::llhttp_on_header_value(llhttp_t *p, const char *at, size_t len) {
  request *req = (request *)p->data;
  req->m_headers.back().second = std::string_view(at, len);
  return HPE_OK;
}

const char *status_to_string(status s) {
  switch (s) {
  // 1xx: Informational
  //
  case status::CONTINUE: return "Continue";
  case status::SWITCHING_PROTOCOLS: return "Switching Protocols";
  case status::PROCESSING: return "Processing";
  case status::EARLY_HINTS: return "Early Hints";

  // 2xx: Success
  //
  case status::OK: return "OK";
  case status::CREATED: return "Created";
  case status::ACCEPTED: return "Accepted";
  case status::NON_AUTHORITATIVE_INFORMATION: return "Non-Authoritative Information";
  case status::NO_CONTENT: return "No Content";
  case status::RESET_CONTENT: return "Reset Content";
  case status::PARTIAL_CONTENT: return "Partial Content";
  case status::MULTI_STATUS: return "Multi-Status";
  case status::ALREADY_REPORTED: return "Already Reported";
  case status::IM_USED: return "IM Used";

  // 3xx: Redirection
  //
  case status::MULTIPLE_CHOICES: return "Multiple Choices";
  case status::MOVED_PERMANENTLY: return "Moved Permanently";
  case status::FOUND: return "Found";
  case status::SEE_OTHER: return "See Other";
  case status::NOT_MODIFIED: return "Not Modified";
  case status::USE_PROXY: return "Use Proxy";
  case status::TEMPORARY_REDIRECT: return "Temporary Redirect";
  case status::PERMANENT_REDIRECT: return "Permanent Redirect";

  // 4xx: Client Errors
  //
  case status::BAD_REQUEST: return "Bad Request";
  case status::UNAUTHORIZED: return "Unauthorized";
  case status::PAYMENT_REQUIRED: return "Payment Required";
  case status::FORBIDDEN: return "Forbidden";
  case status::NOT_FOUND: return "Not Found";
  case status::METHOD_NOT_ALLOWED: return "Method Not Allowed";
  case status::NOT_ACCEPTABLE: return "Not Acceptable";
  case status::PROXY_AUTHENTICATION_REQUIRED: return "Proxy Authentication Required";
  case status::REQUEST_TIMEOUT: return "Request Timeout";
  case status::CONFLICT: return "Conflict";
  case status::GONE: return "Gone";
  case status::LENGTH_REQUIRED: return "Length Required";
  case status::PRECONDITION_FAILED: return "Precondition Failed";
  case status::PAYLOAD_TOO_LARGE: return "Payload Too Large";
  case status::URI_TOO_LONG: return "URI Too Long";
  case status::UNSUPPORTED_MEDIA_TYPE: return "Unsupported Media Type";
  case status::RANGE_NOT_SATISFIABLE: return "Range Not Satisfiable";
  case status::EXPECTATION_FAILED: return "Expectation Failed";
  case status::IM_A_TEAPOT: return "I'm a teapot";
  case status::MISDIRECTED_REQUEST: return "Misdirected Request";
  case status::UNPROCESSABLE_ENTITY: return "Unprocessable Entity";
  case status::LOCKED: return "Locked";
  case status::FAILED_DEPENDENCY: return "Failed Dependency";
  case status::TOO_EARLY: return "Too Early";
  case status::UPGRADE_REQUIRED: return "Upgrade Required";
  case status::PRECONDITION_REQUIRED: return "Precondition Required";
  case status::TOO_MANY_REQUESTS: return "Too Many Requests";
  case status::REQUEST_HEADER_FIELDS_TOO_LARGE: return "Request Header Fields Too Large";
  case status::UNAVAILABLE_FOR_LEGAL_REASONS: return "Unavailable For Legal Reasons";

  // 5xx: Server Errors
  //
  case status::INTERNAL_SERVER_ERROR: return "Internal Server Error";
  case status::NOT_IMPLEMENTED: return "Not Implemented";
  case status::BAD_GATEWAY: return "Bad Gateway";
  case status::SERVICE_UNAVAILABLE: return "Service Unavailable";
  case status::GATEWAY_TIMEOUT: return "Gateway Timeout";
  case status::HTTP_VERSION_NOT_SUPPORTED: return "HTTP Version Not Supported";
  case status::VARIANT_ALSO_NEGOTIATES: return "Variant Also Negotiates";
  case status::INSUFFICIENT_STORAGE: return "Insufficient Storage";
  case status::LOOP_DETECTED: return "Loop Detected";
  case status::NOT_EXTENDED: return "Not Extended";
  case status::NETWORK_AUTHENTICATION_REQUIRED: return "Network Authentication Required";

  default: return "Unknown HTTP Status";
  }
}

} // namespace fc
