#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <sys/param.h>
#include <utility>
#include <uv.h>
#include <uv/unix.h>
#include <vector>

#include "external/llhttp/llhttp.h"

#include "consts.h"
#include "http.hpp"
#include "include/fc.hpp"
#include "router.hpp"
#include "signals.h"
#include "utils.hpp"

#define FC_BACKLOG (128)
#define MAX_REQ_LEN (1024 * 1024 * 5) // 5 MB

namespace fc {

struct app::impl {
public:
  root_router m_router;
  http_parser m_http_parser;

  uv_loop_t *m_loop;
  uv_tcp_t m_host_sock;
  struct sockaddr_in m_addr;

  impl() : m_router(), m_http_parser(), m_loop(uv_default_loop()) {
    m_loop->data = this;
    uv_tcp_init(m_loop, &m_host_sock);
  }

  void parse_http_request(request);
  void match_request_to_handler(request);
  void send_response(request, response);
  bool try_serve_static_file(request);
  void send_file(const char *path_buf, uv_stream_t *remote);

  // uv callbacks
  static void on_close_conn(uv_handle_t *client);
  static void on_open_file(uv_fs_t *);
  static void on_read_file_chunk(uv_fs_t *);
  static void on_connection(uv_stream_t *server, int status);
  static void on_write_buf(uv_write_t *req, int status);
  static void on_write_and_close(uv_write_t *req, int status);
  static void on_alloc_buf(uv_handle_t *client, size_t size, uv_buf_t *buf);
  static void on_read_buf(uv_stream_t *client, long nread, const uv_buf_t *buf);

  void add_route(method, const std::string, path_handler, const std::vector<path_handler> &);
};

struct send_file_ctx {
public:
  int m_file_fd;
  uv_stream_t *m_remote;
  char m_chunck[64 * 1024]; // 64KB

  send_file_ctx(int fd, uv_stream_t *remote) : m_file_fd(fd), m_remote(remote) {
    reset_chunk();
  };

  ~send_file_ctx() {
    if (m_file_fd != -1) {
      uv_fs_t close_req;
      uv_fs_close(uv_default_loop(), &close_req, m_file_fd, nullptr);
    }
  }

  void reset_chunk() {
    memset(m_chunck, 0, sizeof(m_chunck));
  }
};

app::app() {
  signals::ignore_sigpipe(); // must be called before any uv functions
  m_pimpl = new app::impl();
}

app::~app() { delete m_pimpl; }

void app::get(const std::string path, path_handler handler) {
  m_pimpl->add_route(method::GET, path, handler, {});
}

void app::post(const std::string path, path_handler handler) {
  m_pimpl->add_route(method::POST, path, handler, {});
}

void app::put(const std::string path, path_handler handler) {
  m_pimpl->add_route(method::PUT, path, handler, {});
}

void app::delet(const std::string path, path_handler handler) {
  m_pimpl->add_route(method::DELETE, path, handler, {});
}

void app::patch(const std::string path, path_handler handler) {
  m_pimpl->add_route(method::PATCH, path, handler, {});
}

void app::use(const router &router) {
  for (auto &r : router.m_routes) {
    m_pimpl->add_route(r.m_method, router.m_base + r.m_path, r.m_handler, router.m_midwares);
  }
}

void app::impl::add_route(method method, const std::string path, path_handler handler, const std::vector<path_handler> &midwares) {
  m_router.add(method, path, handler, midwares);
}

int app::listen(const std::string addr, std::function<void(const std::string &)> call_back) {
  auto [host, port] = split_address(addr);
  uv_ip4_addr(host.c_str(), std::stoi(port), &m_pimpl->m_addr);
  int result = uv_tcp_bind(&m_pimpl->m_host_sock, (const struct sockaddr *)&m_pimpl->m_addr, 0);
  if (result) {
    std::cerr << "[FALCON ERROR]: Failed to bind at " << addr << ", " << uv_strerror(result) << std::endl;
    return -1;
  }
  result = uv_listen((uv_stream_t *)&m_pimpl->m_host_sock, FC_BACKLOG, app::impl::on_connection);
  if (result) {
    std::cerr << "[FALCON ERROR]: Failed to listen at " << addr << ", " << uv_strerror(result) << std::endl;
    return -1;
  }
  if (call_back) call_back(host + ":" + port);
  return uv_run(m_pimpl->m_loop, UV_RUN_DEFAULT);
}

void app::impl::on_connection(uv_stream_t *host, int status) {
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
  uv_read_start((uv_stream_t *)remote, app::impl::on_alloc_buf, app::impl::on_read_buf);
}

void app::impl::on_alloc_buf(uv_handle_t *client, size_t len, uv_buf_t *buf) {
  buf->len = len;
  buf->base = new char[len];
  std::memset(buf->base, 0, len);
}

void app::impl::on_read_buf(uv_stream_t *client, long nread, const uv_buf_t *buf) {
  uv_read_stop(client);
  if (nread < 0) {
    if (nread != UV_EOF)
      std::cerr << "[FALCON ERROR]: Failed to read remote socket, " << uv_strerror(nread) << std::endl;
    delete[] buf->base;
    uv_close((uv_handle_t *)client, app::impl::on_close_conn);
  } else {
    auto this_ = (app::impl *)client->loop->data;
    request req = request(buf->base, (void *)client);
    this_->parse_http_request(req);
  }
}

void app::impl::parse_http_request(request req) {
  enum llhttp_errno err = m_http_parser.parse(&req);
  if (HPE_OK != err) {
    std::cerr << "[FALCON ERROR]: Faild to parse request, " << llhttp_errno_name(err) << std::endl;
    return send_response(std::move(req), response::ok(status::BAD_REQUEST));
  }
  match_request_to_handler(std::move(req));
}

void app::impl::match_request_to_handler(request req) {
  if (m_router.match(req)) {
    return send_response(std::move(req), req.next());
  }
  if (method::GET != req.m_method || !try_serve_static_file(std::move(req))) {
    return send_response(std::move(req), response::ok(status::NOT_FOUND));
  }
}

bool app::impl::try_serve_static_file(request req) {
  auto path = validate_and_resolve_path(std::string(FC_PUBLIC_DIR), std::string(req.m_path));
  if (path.has_value()) {
    auto res = response(status::OK, path->string(), true);
    res.set_header("Transfer-Encoding", "chunked");
    res.set_header("Content-Type", get_content_from_extension(path->extension().string()));
    send_response(std::move(req), std::move(res));
    return true;
  }
  return false;
}

void app::impl::send_response(request req, response res) {
  size_t headers_len = 0;
  for (const auto &header : res.m_headers) {
    headers_len += header.first.length() + header.second.length() + 4;
  }
  char headers[headers_len + 1];
  size_t headers_offset = 0;
  for (const auto &header : res.m_headers) {
    std::memcpy(headers + headers_offset, header.first.data(), header.first.length());
    headers_offset += header.first.length();
    std::memcpy(headers + headers_offset, ": ", 2);
    headers_offset += 2;
    std::memcpy(headers + headers_offset, header.second.data(), header.second.length());
    headers_offset += header.second.length();
    std::memcpy(headers + headers_offset, "\r\n", 2);
    headers_offset += 2;
  }
  headers[headers_offset] = '\0';

  auto statstr = status_to_string(res.m_status);
  auto header_len = snprintf(nullptr, 0, http_header_cfmt, (int)res.m_status, statstr, headers);
  auto header_buf = new char[header_len + 1];
  snprintf(header_buf, header_len + 1, http_header_cfmt, (int)res.m_status, statstr, headers);

  uv_buf_t write_buf = uv_buf_init(header_buf, header_len);
  uv_write_t *write_req = new uv_write_t;
  write_req->data = header_buf;
  uv_write(write_req, (uv_stream_t *)req.m_uvsock, &write_buf, 1, app::impl::on_write_buf);

  if (res.m_isfile) {
    app::impl::send_file(cstr_from_string(res.m_body), (uv_stream_t *)req.m_uvsock);
  } else {
    auto body_len = res.m_body.length();
    auto body_buf = new char[body_len + 1];
    std::memcpy(body_buf, res.m_body.data(), body_len);
    body_buf[body_len] = '\0';

    uv_buf_t body_write_buf = uv_buf_init(body_buf, body_len);
    uv_write_t *body_write_req = new uv_write_t;
    body_write_req->data = body_buf;
    uv_write(body_write_req, (uv_stream_t *)req.m_uvsock, &body_write_buf, 1, app::impl::on_write_and_close);
  }
}

void app::impl::send_file(const char *path_buf, uv_stream_t *remote) {
  uv_fs_t *open_req = new uv_fs_t;
  open_req->data = new send_file_ctx(-1, remote);
  uv_fs_open(uv_default_loop(), open_req, path_buf, O_RDONLY, 0, app::impl::on_open_file);
}

void app::impl::on_open_file(uv_fs_t *open_req) {
  auto ctx = (send_file_ctx *)open_req->data;

  if (open_req->result < 0) {
    std::cerr << "[FALCON ERROR]: Failed to open file, " << uv_strerror(open_req->result) << std::endl;
  } else {
    ctx->m_file_fd = open_req->result;
    uv_fs_t *read_req = new uv_fs_t;
    read_req->data = ctx;
    uv_buf_t read_buf = uv_buf_init(ctx->m_chunck, sizeof(ctx->m_chunck));
    uv_fs_read(uv_default_loop(), read_req, ctx->m_file_fd, &read_buf, 1, -1, app::impl::on_read_file_chunk);
  }

  delete[] open_req->path;
  delete open_req;
}

void app::impl::on_read_file_chunk(uv_fs_t *read_req) {
  auto *ctx = (send_file_ctx *)read_req->data;

  if (read_req->result > 0) {
    size_t chunk_len = snprintf(nullptr, 0, "%x\r\n%s\r\n", (unsigned int)read_req->result, read_req->bufs[0].base);
    char *chunk_buf = new char[chunk_len + 1];
    snprintf(chunk_buf, chunk_len + 1, "%x\r\n%s\r\n", (unsigned int)read_req->result, read_req->bufs[0].base);

    uv_buf_t write_buf = uv_buf_init(chunk_buf, chunk_len);
    uv_write_t *write_req = new uv_write_t;
    write_req->data = chunk_buf;
    uv_write(write_req, ctx->m_remote, &write_buf, 1, app::impl::on_write_buf);

    // read next chunk
    ctx->reset_chunk();
    uv_fs_read(uv_default_loop(), read_req, ctx->m_file_fd, read_req->bufs, 1, -1, app::impl::on_read_file_chunk);
  } else {
    if (read_req->result < 0) {
      fprintf(stderr, "[FALCON ERROR]: Failed to read from static file, %s\n", uv_strerror(read_req->result));
    }

    static char *last_chunk = (char *)"0\r\n\r\n";
    auto last_chunk_buf = uv_buf_init(last_chunk, strlen(last_chunk));
    uv_write_t *write_req = new uv_write_t;
    write_req->data = nullptr;
    uv_write(write_req, ctx->m_remote, &last_chunk_buf, 1, app::impl::on_write_and_close);

    delete ctx;
    delete read_req;
  }
}

void app::impl::on_write_buf(uv_write_t *req, int status) {
  if (status < 0) {
    std::cerr << "[FALCON ERROR]: Failed to write response buf, " << uv_strerror(status) << std::endl;
  }
  if (req->data) delete[] (char *)req->data;
  delete req;
}

void app::impl::on_write_and_close(uv_write_t *req, int status) {
  auto remote = (uv_handle_t *)req->handle;
  on_write_buf(req, status);
  uv_close(remote, app::impl::on_close_conn);
}

void app::impl::on_close_conn(uv_handle_t *client) {
  delete client;
}

} // namespace fc
