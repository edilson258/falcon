#pragma once

#include "falcon.h"
#include "llhttp.h"

namespace fc {

struct http_parser {
  llhttp_t m_llhttp_instance{};
  llhttp_settings_t m_llhttp_settings{};

  http_parser() {
    llhttp_settings_init(&m_llhttp_settings);
    m_llhttp_settings.on_url = llhttp_on_url;
    m_llhttp_settings.on_method = llhttp_on_method;
    m_llhttp_settings.on_body = llhttp_on_body;
    m_llhttp_settings.on_header_field = llhttp_on_header_field;
    m_llhttp_settings.on_header_value = llhttp_on_header_value;
    llhttp_init(&m_llhttp_instance, HTTP_REQUEST, &m_llhttp_settings);
  }

  llhttp_errno parse(request *);

  static int llhttp_on_url(const llhttp_t *p, const char *at, size_t len);
  static int llhttp_on_method(const llhttp_t *p, const char *at, size_t len);
  static int llhttp_on_body(const llhttp_t *p, const char *at, size_t len);
  static int llhttp_on_header_field(const llhttp_t *p, const char *at, size_t len);
  static int llhttp_on_header_value(const llhttp_t *p, const char *at, size_t len);
};

const char *status_to_string(status);

} // namespace fc
