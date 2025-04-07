#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <uv.h>
#include <uv/unix.h>

#include "external/llhttp/llhttp.h"

#include "http.hpp"
#include "include/fc.hpp"
#include "req.hpp"
#include "router.hpp"
#include "utils.hpp"

#define FC_BACKLOG (128)
#define MAX_REQ_LEN (1024 * 1024 * 5) // 5 MB

namespace fc {

struct app::impl {
public:
  router m_Root;
  http_parser m_HttpParser;

  // The 'm_Loop->data' field always holds a pointer to an object of this struct
  uv_loop_t *m_Loop;
  uv_tcp_t m_HostSock;
  struct sockaddr_in m_Addr;

  impl() : m_Root(), m_HttpParser(), m_Loop(uv_default_loop()) {
    m_Loop->data = this;
    uv_tcp_init(m_Loop, &m_HostSock);
  }

  void ParseHttpRequest(request);
  void MatchRequestToHandler(request);
  void HandleResponse(request, response);

  void AddRoute(method method, const std::string path, path_handler handler);

  // uv callbacks
  static void OnConnection(uv_stream_t *server, int status);
  static void OnAllocBuf(uv_handle_t *client, size_t size, uv_buf_t *buf);
  static void OnReadBuf(uv_stream_t *client, long nread, const uv_buf_t *buf);
  static void OnWrite(uv_write_t *req, int status);
  static void OnCloseConn(uv_handle_t *client);
};

app::app() : m_PImpl(new app::impl()) {}
app::~app() { delete m_PImpl; }

void app::get(const std::string path, path_handler handler) {
  m_PImpl->AddRoute(method::GET, path, handler);
}

void app::post(const std::string path, path_handler handler) {
  m_PImpl->AddRoute(method::POST, path, handler);
}

void app::put(const std::string path, path_handler handler) {
  m_PImpl->AddRoute(method::PUT, path, handler);
}

void app::delet(const std::string path, path_handler handler) {
  m_PImpl->AddRoute(method::DELETE, path, handler);
}

void app::patch(const std::string path, path_handler handler) {
  m_PImpl->AddRoute(method::PATCH, path, handler);
}

int app::listen(const std::string addr, std::function<void()> callBack) {
  auto [host, port] = split_address(addr);
  uv_ip4_addr(host.c_str(), std::stoi(port), &m_PImpl->m_Addr);
  int result = uv_tcp_bind(&m_PImpl->m_HostSock, (const struct sockaddr *)&m_PImpl->m_Addr, 0);
  if (result) {
    std::cerr << "[FALCON ERROR]: Failed to bind at " << addr << ", " << uv_strerror(result) << std::endl;
    return -1;
  }
  result = uv_listen((uv_stream_t *)&m_PImpl->m_HostSock, FC_BACKLOG, app::impl::OnConnection);
  if (result) {
    std::cerr << "[FALCON ERROR]: Failed to listen at " << addr << ", " << uv_strerror(result) << std::endl;
    return -1;
  }
  callBack();
  return uv_run(m_PImpl->m_Loop, UV_RUN_DEFAULT);
}

void app::impl::OnConnection(uv_stream_t *host, int status) {
  if (status < 0) {
    std::cerr << "[FALCON ERROR]: Failed to accept new connection, " << uv_strerror(status) << std::endl;
    return;
  }
  uv_tcp_t *remote = new uv_tcp_t;
  uv_tcp_init(host->loop, remote);
  int result = uv_accept(host, (uv_stream_t *)remote);
  if (result != 0) {
    std::cerr << "[FALCON ERROR]: Failed to accept new connection, " << uv_strerror(result) << std::endl;
    return;
  }
  uv_read_start((uv_stream_t *)remote, app::impl::OnAllocBuf, app::impl::OnReadBuf);
}

void app::impl::OnAllocBuf(uv_handle_t *client, size_t size, uv_buf_t *buf) {
  buf->len = size;
  buf->base = new char[size];
}

void app::impl::OnReadBuf(uv_stream_t *client, long nread, const uv_buf_t *buf) {
  uv_read_stop(client);
  if (nread < 0) {
    if (nread != UV_EOF)
      std::cerr << "[FALCON ERROR]: Failed to read remote socket, " << uv_strerror(nread) << std::endl;
    delete[] buf->base;
    uv_close((uv_handle_t *)client, app::impl::OnCloseConn);
    return;
  }
  ((app::impl *)client->loop->data)->ParseHttpRequest(request_factory((void *)client, std::string_view(buf->base, strnlen(buf->base, MAX_REQ_LEN))));
}

void app::impl::ParseHttpRequest(request req) {
  enum llhttp_errno err = m_HttpParser.parse(req);
  if (HPE_OK != err) {
    // send 'bad request' response
    return;
  }
  MatchRequestToHandler(req);
}

void app::impl::MatchRequestToHandler(request req) {
  auto handler = m_Root.match(req.get_method(), req.get_path(), req);
  if (handler) {
    auto res = (handler)(req);
    HandleResponse(req, res);
  } else {
    std::cout << "No match\n";
  }
}

void app::impl::HandleResponse(request r, response res) {
  uv_buf_t *write_buf = new uv_buf_t;
  *write_buf = uv_buf_init((char *)res.to_string().c_str(), res.to_string().length());
  uv_write_t *write_req = new uv_write_t;
  uv_write(write_req, (uv_stream_t *)r.get_remote(), write_buf, 1, app::impl::OnWrite);
}

void app::impl::AddRoute(method method, const std::string path, path_handler handler) {
  m_Root.add(method, path, handler);
}

void app::impl::OnWrite(uv_write_t *req, int status) {
  delete req->bufs;
  delete req;
  uv_close((uv_handle_t *)req->handle, app::impl::OnCloseConn);
}

void app::impl::OnCloseConn(uv_handle_t *client) {
  delete client;
}

} // namespace fc
