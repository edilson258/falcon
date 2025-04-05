#pragma once

#include "fc.hpp"
#include "llhttp.h"

namespace fc
{

struct HttpParser
{
public:
  llhttp_t m_llhttpInstance;
  llhttp_settings_t m_llhttpSettings;

  HttpParser()
  {
    llhttp_settings_init(&m_llhttpSettings);
    m_llhttpSettings.on_url = HttpParser::llhttpOnURL;
    m_llhttpSettings.on_method = HttpParser::llhttpOnMethod;
    m_llhttpSettings.on_body = HttpParser::llhttpOnBody;
    m_llhttpSettings.on_header_field = HttpParser::llhttpOnHeaderField;
    m_llhttpSettings.on_header_value = HttpParser::llhttpOnHeaderValue;
    llhttp_init(&m_llhttpInstance, HTTP_REQUEST, &m_llhttpSettings);
  }

  enum llhttp_errno Parse(Req &);

  static int llhttpOnURL(llhttp_t *p, const char *at, size_t len);
  static int llhttpOnMethod(llhttp_t *p, const char *at, size_t len);
  static int llhttpOnBody(llhttp_t *p, const char *at, size_t len);
  static int llhttpOnHeaderField(llhttp_t *p, const char *at, size_t len);
  static int llhttpOnHeaderValue(llhttp_t *p, const char *at, size_t len);
};

} // namespace fc
