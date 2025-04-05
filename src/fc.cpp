#include <array>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <uv.h>
#include <uv/unix.h>

#include "llhttp.h"

#include "fc.hpp"
#include "http.hpp"
#include "req.hpp"
#include "router.hpp"
#include "utils.hpp"

#define FC_BACKLOG (128)
#define MAX_REQ_LEN (1024 * 1024 * 5) // 5 MB

namespace fc
{

struct App::impl
{
public:
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

  void ParseHttpRequest(Req);
  void MatchRequestToHandler(Req);
  void HandleResponse(Req, Res);

  void AddRoute(Method method, const std::string path, PathHandler handler);

  // uv callbacks
  static void OnConnection(uv_stream_t *server, int status);
  static void OnAllocBuf(uv_handle_t *client, size_t size, uv_buf_t *buf);
  static void OnReadBuf(uv_stream_t *client, long nread, const uv_buf_t *buf);
  static void OnWrite(uv_write_t *req, int status);
  static void OnCloseConn(uv_handle_t *client);
};

App::App() : m_PImpl(new App::impl()) {}
App::~App() { delete m_PImpl; }

void App::Get(const std::string path, PathHandler handler)
{
  m_PImpl->AddRoute(Method::GET, path, handler);
}

int App::Listen(const std::string addr, std::function<void()> callBack)
{
  auto [host, port] = matchHostAndPort(addr);
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

void App::impl::OnReadBuf(uv_stream_t *client, long nread, const uv_buf_t *buf)
{
  uv_read_stop(client);
  if (nread < 0)
  {
    if (nread != UV_EOF)
    {
      std::cerr << "[FALCON ERROR]: Failed to read remote socket, " << uv_strerror(nread) << std::endl;
    }
    delete[] buf->base;
    uv_close((uv_handle_t *)client, App::impl::OnCloseConn);
    return;
  }
  ((App::impl *)client->loop->data)->ParseHttpRequest(RequestFactory((void *)client, std::string_view(buf->base, strnlen(buf->base, MAX_REQ_LEN))));
}

void App::impl::OnWrite(uv_write_t *req, int status)
{
  delete req->bufs;
  delete req;
  uv_close((uv_handle_t *)req->handle, App::impl::OnCloseConn);
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
  auto handler = m_Root.MatchRoute(req.GetMethod(), req.GetPath());
  if (handler)
  {
    auto res = (handler)(req);
    HandleResponse(req, res);
  }
}

void App::impl::HandleResponse(Req req, Res res)
{
  uv_buf_t *write_buf = new uv_buf_t;
  *write_buf = uv_buf_init((char *)res.GetRaw().c_str(), res.GetRaw().length());
  uv_write_t *write_req = new uv_write_t;
  uv_write(write_req, (uv_stream_t *)req.GetRemoteSock(), write_buf, 1, App::impl::OnWrite);
}

// void send_http_response(uv_handle_t *handler, fc_http_status status, char *cont_type, char *body, size_t body_len)
// {
//   char *status_str = fc__http_status_str(status);
//   size_t head_len = snprintf(NULL, 0, HTTP_RESPONSE_HEADER_TEMPLATE, status, status_str, cont_type, body_len - 1) + 1;
//   char head[head_len];
//   snprintf(head, head_len, HTTP_RESPONSE_HEADER_TEMPLATE, status, status_str, cont_type, body_len - 1);

//   uv_buf_t *write_bufs = (uv_buf_t *)malloc(sizeof(uv_buf_t) * 2);
//   write_bufs[0] = uv_buf_init(head, head_len - 1);
//   write_bufs[1] = uv_buf_init(body, body_len - 1);

//   uv_write_t *write_req = (uv_write_t *)malloc(sizeof(uv_write_t));
//   uv_write(write_req, (uv_stream_t *)handler, write_bufs, 2, on_write);
// }
// void on_write(uv_write_t *req, int status)
// {
//   free(req->bufs);
//   free(req);
//   uv_close((uv_handle_t *)req->handle, on_close_connection);
// }

void App::impl::AddRoute(Method method, const std::string path, PathHandler handler)
{
  m_Root.AddRoute(method, path, handler);
}

void App::impl::OnCloseConn(uv_handle_t *client)
{
  delete client;
}

} // namespace fc
