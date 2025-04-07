#include <cstring>
#include <string_view>

#include "http.hpp"

namespace fc {

enum llhttp_errno http_parser::parse(request &req) {
  m_llhttp_instance.data = &req;
  enum llhttp_errno err = llhttp_execute(&m_llhttp_instance, req.m_raw.data(), req.m_raw.length());
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

std::string_view m_last_header_field;

int http_parser::llhttp_on_header_field(llhttp_t *p, const char *at, size_t len) {
  request *req = (request *)p->data;
  m_last_header_field = std::string_view(at, len);
  return HPE_OK;
}

int http_parser::llhttp_on_header_value(llhttp_t *p, const char *at, size_t len) {
  request *req = (request *)p->data;
  req->m_headers[m_last_header_field] = std::string_view(at, len);
  return HPE_OK;
}

const char *status_to_string(status status) {
  switch (status) {
  case status::OK: return "OK";
  case status::CREATED: return "Created";
  case status::NOT_FOUND: return "Not Found";
  case status::INTERNAL_SERVER_ERROR: return "Internal Server Error";
  }
  return "unknown http status";
}

} // namespace fc
