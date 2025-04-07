#pragma once

#include "external/llhttp/llhttp.h"
#include "include/fc.hpp"

namespace fc {

struct http_parser {
public:
  llhttp_t m_llhttp_instance;
  llhttp_settings_t m_llhttp_settings;

  http_parser() {
    llhttp_settings_init(&m_llhttp_settings);
    m_llhttp_settings.on_url = http_parser::llhttp_on_url;
    m_llhttp_settings.on_method = http_parser::llhttp_on_method;
    m_llhttp_settings.on_body = http_parser::llhttp_on_body;
    m_llhttp_settings.on_header_field = http_parser::llhttp_on_header_field;
    m_llhttp_settings.on_header_value = http_parser::llhttp_on_header_value;
    llhttp_init(&m_llhttp_instance, HTTP_REQUEST, &m_llhttp_settings);
  }

  enum llhttp_errno parse(request *);

  static int llhttp_on_url(llhttp_t *p, const char *at, size_t len);
  static int llhttp_on_method(llhttp_t *p, const char *at, size_t len);
  static int llhttp_on_body(llhttp_t *p, const char *at, size_t len);
  static int llhttp_on_header_field(llhttp_t *p, const char *at, size_t len);
  static int llhttp_on_header_value(llhttp_t *p, const char *at, size_t len);
};

const char *status_to_string(status);

} // namespace fc
