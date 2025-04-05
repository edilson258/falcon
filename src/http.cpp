#include "http.hpp"
#include <cstring>

namespace fc
{

enum llhttp_errno HttpParser::Parse(Req &req)
{
  m_llhttpInstance.data = &req;
  enum llhttp_errno err = llhttp_execute(&m_llhttpInstance, req.m_Raw.data(), req.m_Raw.length());
  llhttp_reset(&m_llhttpInstance);
  return err;
}

int HttpParser::llhttpOnURL(llhttp_t *p, const char *at, size_t len)
{
  Req *req = (Req *)p->data;
  req->m_Path = std::string_view(at, len);
  return HPE_OK;
}

int HttpParser::llhttpOnMethod(llhttp_t *p, const char *at, size_t len)
{
  Req *req = (Req *)p->data;
  if (strncmp("GET", at, len))
  {
    req->m_Method = Method::GET;
  }
  else if (strncmp("POST", at, len))
  {
    req->m_Method = Method::POST;
  }
  else if (strncmp("PUT", at, len))
  {
    req->m_Method = Method::PUT;
  }
  else if (strncmp("DELETE", at, len))
  {
    req->m_Method = Method::DELETE;
  }
  else
  {
    return HPE_INVALID_METHOD;
  }
  return HPE_OK;
}

int HttpParser::llhttpOnBody(llhttp_t *p, const char *at, size_t len)
{
  return HPE_OK;
}

int HttpParser::llhttpOnHeaderField(llhttp_t *p, const char *at, size_t len)
{
  return HPE_OK;
}

int HttpParser::llhttpOnHeaderValue(llhttp_t *p, const char *at, size_t len)
{
  return HPE_OK;
}

} // namespace fc
