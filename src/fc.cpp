#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <iostream>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <uv.h>
#include <vector>

#include "fc.hpp"
#include "llhttp.h"

#define FC_BACKLOG (128)
#define MAX_REQ_LEN (1024 * 1024 * 5) // 5 MB

namespace fc
{
enum class PathFragType
{
  STATIC = 1,
  DYNAMIC = 2,
  WILDCARD = 3,
};

struct PathFrag
{
public:
  PathFragType m_Type;
  std::string m_Label;
  std::array<PathHandler, (int)Method::COUNT> m_Handlers;

  PathFrag *m_Next;
  PathFrag *m_Child;

  PathFrag() = default;
  PathFrag(PathFragType type, std::string label) : m_Type(type), m_Label(label), m_Next(nullptr), m_Child(nullptr)
  {
    std::fill(m_Handlers.begin(), m_Handlers.end(), nullptr);
  };
};

struct Router
{
public:
  PathFrag m_Root;

  Router() = default;

  void AddRoute(Method method, const std::string, PathHandler);
  PathHandler MatchRoute(Method method, const std::string_view &) const;

  static std::vector<std::string> FragmentPath(const std::string_view &);
  static std::string NormalizePath(const std::string_view &);
};

std::string Router::NormalizePath(const std::string_view &input)
{
  bool prevSlash = false;
  std::string output;
  for (auto chr : input)
  {
    if (('/' == chr && prevSlash) || std::isspace(chr))
    {
      continue;
    }
    prevSlash = chr == '/' ? true : false;
    output.push_back(chr);
  }
  return output;
}

std::vector<std::string> Router::FragmentPath(const std::string_view &path)
{
  std::vector<std::string> frags;
  auto normalPath = NormalizePath(path);
  char *frag = std::strtok((char *)normalPath.c_str(), "/");
  while (frag)
  {
    frags.push_back(frag);
    frag = std::strtok(nullptr, "/");
  }
  return frags;
}

void Router::AddRoute(Method method, const std::string path, PathHandler handler)
{
  auto pathFragments = FragmentPath(path);
  PathFrag *current = &m_Root;

  for (const auto &frag : pathFragments)
  {
    PathFragType type;
    switch (frag.at(0))
    {
    case '*': type = PathFragType::WILDCARD; break;
    case ':': type = PathFragType::DYNAMIC; break;
    default: type = PathFragType::STATIC; break;
    }

    bool found = false;
    PathFrag *prev = nullptr;
    PathFrag *child = current->m_Child;
    while (child)
    {
      if ((type == PathFragType::STATIC && child->m_Label == frag) || (type != PathFragType::STATIC && child->m_Type == type))
      {
        if (type == PathFragType::DYNAMIC && child->m_Label != frag)
        {
          throw std::runtime_error("Conflicting dynamic segment names: " + child->m_Label + " vs " + frag);
        }
        found = true;
        break;
      }
      prev = child;
      child = child->m_Next;
    }
    if (!found)
    {
      PathFrag *newFrag = new PathFrag(type, frag);
      if (prev)
      {
        prev->m_Next = newFrag;
      }
      else
      {
        current->m_Child = newFrag;
      }
      child = newFrag;
    }
    current = child;
  }
  if (current->m_Handlers[static_cast<int>(method)] != nullptr)
  {
    throw std::runtime_error("Duplicate route for method " + std::to_string(static_cast<int>(method)) + " at path: " + path);
  }
  current->m_Handlers[static_cast<int>(method)] = handler;
}

struct MatchResult
{
  bool m_HasMatch;
  PathFragType m_Type;
  std::unordered_map<std::string, std::string> m_DynParams;
};

PathHandler Router::MatchRoute(Method method, const std::string_view &path) const
{
  auto fragments = FragmentPath(path);
  const PathFrag *current = &m_Root;
  for (auto &frag : fragments)
  {
    bool found = false;
    const PathFrag *child = current->m_Child;
    while (child && !found)
    {
      switch (child->m_Type)
      {
      case PathFragType::STATIC: found = frag == child->m_Label; break;
      case PathFragType::DYNAMIC: found = true; break;
      case PathFragType::WILDCARD: return child->m_Handlers[static_cast<int>(method)];
      }
      if (found)
      {
        current = child;
        break;
      }
      child = child->m_Next;
    }
    if (!found)
    {
      return nullptr;
    }
  }
  std::cout << (int)method << std::endl;
  return current->m_Handlers.at((int)method);
}

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

struct App::impl
{
public:
  // router
  Router m_Root;
  HttpParser m_HttpParser;

  uv_loop_t *m_Loop;
  uv_tcp_t m_HostSock;
  struct sockaddr_in m_Addr;

  impl() : m_Root(), m_HttpParser(), m_Loop(uv_default_loop())
  {
    m_Loop->data = this;
    uv_tcp_init(m_Loop, &m_HostSock);
  }

  void AddRoute(Method method, const std::string path, PathHandler handler) { m_Root.AddRoute(method, path, handler); }

  // uv callbacks
  static void OnConnection(uv_stream_t *server, int status);
  static void OnAllocBuf(uv_handle_t *client, size_t size, uv_buf_t *buf);
  static void OnReadBuf(uv_stream_t *client, long nread, const uv_buf_t *buf);
  static void OnCloseConn(uv_handle_t *client);

  // http parser
  void ParseHttpRequest(Req);
  void MatchRequestToHandler(Req);
};

App::App() : m_PImpl(new App::impl()) {}
App::~App() { delete m_PImpl; }

Req RequestFactory(void *remote, std::string_view sv) { return Req(remote, sv); }

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
  req->m_Method = Method::GET;
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

/*
 * Response
 */

Res Res::Ok()
{
  return Res();
}

void App::impl::OnConnection(uv_stream_t *host, int status)
{
  if (status < 0)
  {
    std::cerr << "[FALCON ERROR]: Failed to accept new connection, " << uv_strerror(status) << std::endl;
    return;
  }
  uv_tcp_t *remote = new uv_tcp_t;
  uv_tcp_init(host->loop, remote);
  int result = uv_accept(host, (uv_stream_t *)remote);
  if (result != 0)
  {
    std::cerr << "[FALCON ERROR]: Failed to accept new connection, " << uv_strerror(result) << std::endl;
    return;
  }
  uv_read_start((uv_stream_t *)remote, App::impl::OnAllocBuf, App::impl::OnReadBuf);
}

void App::impl::OnAllocBuf(uv_handle_t *client, size_t size, uv_buf_t *buf)
{
  buf->len = size;
  buf->base = new char[size];
}

void App::impl::ParseHttpRequest(Req req)
{
  enum llhttp_errno err = m_HttpParser.Parse(req);
  if (HPE_OK != err)
  {
    // send 'bad request' response
    return;
  }
  MatchRequestToHandler(req);
}

void App::impl::MatchRequestToHandler(Req req)
{
  std::cout << (int)req.GetMethod() << std::endl;
  auto handler = m_Root.MatchRoute(req.GetMethod(), req.GetPath());
  if (handler)
  {
    (handler)(req);
  }
}

void App::impl::OnReadBuf(uv_stream_t *client, long nread, const uv_buf_t *buf)
{
  if (nread < 0)
  {
    std::cerr << "[FALCON ERROR]: Failed to read remote socket, " << uv_strerror(nread) << std::endl;
    return;
  }
  ((App::impl *)client->loop->data)->ParseHttpRequest(RequestFactory((void *)client, std::string_view(buf->base, strnlen(buf->base, MAX_REQ_LEN))));
}

void App::impl::OnCloseConn(uv_handle_t *client) { delete client; }

void App::Get(const std::string path, PathHandler handler)
{
  m_PImpl->AddRoute(Method::GET, path, handler);
}

std::optional<std::tuple<std::string, std::string>> matchIpAndPort(const std::string &input)
{
  std::smatch match;
  static const std::regex pattern(R"((.*?):?(\d+))");
  if (std::regex_match(input, match, pattern))
  {
    return std::tuple<std::string, std::string>(match[1].str().empty() ? "0.0.0.0" : match[1].str(), match[2].str());
  }
  return std::nullopt;
}

int App::Listen(const std::string addr, std::function<void()> callBack)
{
  auto matchAddrOpt = matchIpAndPort(addr);
  if (!matchAddrOpt.has_value())
  {
    std::cerr << "[FALCON ERROR]: Invalid address " << addr << ", try \":8000\"" << std::endl;
    return -1;
  }
  auto [host, port] = matchAddrOpt.value();
  uv_ip4_addr(host.c_str(), std::stoi(port), &m_PImpl->m_Addr);
  int result = uv_tcp_bind(&m_PImpl->m_HostSock, (const struct sockaddr *)&m_PImpl->m_Addr, 0);
  if (result)
  {
    std::cerr << "[FALCON ERROR]: Failed to bind at " << addr << ", " << uv_strerror(result) << std::endl;
    return -1;
  }
  result = uv_listen((uv_stream_t *)&m_PImpl->m_HostSock, FC_BACKLOG, App::impl::OnConnection);
  if (result)
  {
    std::cerr << "[FALCON ERROR]: Failed to listen at " << addr << ", " << uv_strerror(result) << std::endl;
    return -1;
  }
  callBack();
  return uv_run(m_PImpl->m_Loop, UV_RUN_DEFAULT);
}
} // namespace fc
