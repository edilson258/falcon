#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <memory>
#include <string>
#include <sys/param.h>
#include <utility>
#include <uv.h>
#include <uv/unix.h>
#include <vector>

#include "external/llhttp/llhttp.h"

#include "const.hpp"
#include "debug.hpp"
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
  std::string m_views_dir = "views/";
  std::string m_assets_dir = "public/";

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

  // reponse handlers
  void send_response(request, response);
  void send_response_file(request, response);
  static void send_response_head(request &, response &);
  static void send_response_body(request, response);

  // uv callbacks
  static void on_close_conn(uv_handle_t *client);
  static void on_open_file(uv_fs_t *);
  static void on_read_file_chunk(uv_fs_t *);
  static void on_connection(uv_stream_t *server, int status);
  static void on_write_buf(uv_write_t *req, int status);
  static void on_write_and_close(uv_write_t *req, int status);
  static void on_alloc_req_buf(uv_handle_t *client, size_t size, uv_buf_t *buf);
  static void on_read_req_buf(uv_stream_t *client, long nread, const uv_buf_t *buf);

  void add_route(method, std::string, path_handler, const std::vector<path_handler> &);
};

struct send_file_ctx {
public:
  int m_file_fd;
  uv_stream_t *m_remote;
  char m_chunk[64 * 1024]; // 64KB

  std::optional<request> m_req;
  std::optional<response> m_res;

  send_file_ctx(int fd, uv_stream_t *sock, request req, response res) : m_file_fd(fd), m_remote(sock), m_req(std::move(req)), m_res(std::move(res)) {
    reset_chunk();
  };

  ~send_file_ctx() {
    if (m_file_fd != -1) {
      uv_fs_t close_req;
      uv_fs_close(uv_default_loop(), &close_req, m_file_fd, nullptr);
    }
  }

  void reset_chunk() {
    memset(m_chunk, 0, sizeof(m_chunk));
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

void app::set_views_dir(const std::string dir_path) {
  m_pimpl->m_views_dir = dir_path;
}

void app::set_assets_dir(const std::string dir_path) {
  m_pimpl->m_assets_dir = dir_path;
}

void app::impl::add_route(method method, std::string path, path_handler handler, const std::vector<path_handler> &midwares) {
  if (path.length() < 1 || path.at(0) != '/') path.insert(0, "/");
  m_router.add(method, path, handler, midwares);
}

int app::listen(const std::string addr, std::function<void(const std::string &)> call_back) {
  auto [host, port] = split_address(addr);
  uv_ip4_addr(host.c_str(), std::stoi(port), &m_pimpl->m_addr);
  int result = uv_tcp_bind(&m_pimpl->m_host_sock, (const struct sockaddr *)&m_pimpl->m_addr, 0);
  if (result) {
    debug::error("Fail to bind at %s, %s", addr.c_str(), uv_strerror(result));
    return -1;
  }
  result = uv_listen((uv_stream_t *)&m_pimpl->m_host_sock, FC_BACKLOG, app::impl::on_connection);
  if (result) {
    debug::error("Fail to listen at %s, %s", addr.c_str(), uv_strerror(result));
    return -1;
  }
  if (call_back) call_back(host + ":" + port);
  debug::info("Event loop spinned");
  return uv_run(m_pimpl->m_loop, UV_RUN_DEFAULT);
}

void app::impl::on_connection(uv_stream_t *host, int status) {
  if (status < 0) return debug::error("Fail to accept new connection, %s", uv_strerror(status));
  uv_tcp_t *remote = new uv_tcp_t;
  uv_tcp_init(host->loop, remote);
  if (int result = uv_accept(host, (uv_stream_t *)remote); 0 != result)
    return debug::error("Fail to accept new connection, %s", uv_strerror(result));
  uv_read_start((uv_stream_t *)remote, app::impl::on_alloc_req_buf, app::impl::on_read_req_buf);
}

void app::impl::on_alloc_req_buf(uv_handle_t *client, size_t len, uv_buf_t *buf) {
  buf->len = len;
  buf->base = new char[len];
}

void app::impl::on_read_req_buf(uv_stream_t *client, long nread, const uv_buf_t *buf) {
  uv_read_stop(client);
  if (nread < 0) {
    if (nread != UV_EOF)
      std::cerr << "[FALCON ERROR]: Failed to read remote socket, " << uv_strerror(nread) << std::endl;
    // delete[] buf->base;
    uv_close((uv_handle_t *)client, app::impl::on_close_conn);
  } else {
    auto this_ = (app::impl *)client->loop->data;
    std::unique_ptr<char[]> xs(buf->base);
    this_->parse_http_request(request((void *)client, std::move(xs)));
  }
}

void app::impl::parse_http_request(request req) {
  enum llhttp_errno err = m_http_parser.parse(&req);
  if (HPE_OK != err) {
    debug::error("Fail to parse http request, %s", llhttp_errno_name(err));
    return send_response(std::move(req), response::ok(status::BAD_REQUEST));
  }
  match_request_to_handler(std::move(req));
}

void app::impl::match_request_to_handler(request req) {
  if (m_router.match(req)) {
    return send_response(std::move(req), req.next());
  }

  if (method::GET != req.m_method) {
    return send_response(std::move(req), response::ok(status::NOT_FOUND));
  }

  auto path = join_paths(m_assets_dir, std::string(req.m_path));
  auto res = response(status::OK, response::file_info(path.string()));
  res.set_header("Content-Type", contype_from_ext(path.extension().string()));
  send_response_file(std::move(req), std::move(res));
}

void app::impl::send_response_head(request &req, response &res) {
  size_t headers_len = 0;
  size_t headers_offs = 0;
  for (const auto &h : res.m_headers) {
    headers_len += h.first.length() + h.second.length() + 4;
  }
  char headers[headers_len + 1];
  for (const auto &h : res.m_headers) {
    std::memcpy(headers + headers_offs, h.first.data(), h.first.length());
    headers_offs += h.first.length();
    std::memcpy(headers + headers_offs, ": ", 2);
    headers_offs += 2;
    std::memcpy(headers + headers_offs, h.second.data(), h.second.length());
    headers_offs += h.second.length();
    std::memcpy(headers + headers_offs, "\r\n", 2);
    headers_offs += 2;
  }
  headers[headers_offs] = '\0';

  auto statstr = status_to_string(res.m_status);
  auto header_len = snprintf(nullptr, 0, http_header_cfmt, (int)res.m_status, statstr, headers);
  auto header_buf = new char[header_len + 1];
  snprintf(header_buf, header_len + 1, http_header_cfmt, (int)res.m_status, statstr, headers);

  uv_buf_t write_buf = uv_buf_init(header_buf, header_len);
  uv_write_t *write_req = new uv_write_t;
  write_req->data = header_buf;
  uv_write(write_req, (uv_stream_t *)req.m_remote, &write_buf, 1, app::impl::on_write_buf);
}

void app::impl::send_response_body(request req, response res) {
  auto content_len = res.m_body.length();
  auto content = new char[content_len + 1];
  std::memcpy(content, res.m_body.data(), content_len);
  content[content_len] = '\0';

  auto uv_buf = uv_buf_init(content, content_len);
  auto write_req = new uv_write_t;
  write_req->data = content;
  uv_write(write_req, (uv_stream_t *)req.m_remote, &uv_buf, 1, app::impl::on_write_and_close);
};

void app::impl::send_response(request req, response res) {
  if (res.m_is_file) {
    return send_response_file(std::move(req), std::move(res));
  }
  res.set_header("Content-Len", std::to_string(res.m_body.length()));
  app::impl::send_response_head(req, res);
  app::impl::send_response_body(std::move(req), std::move(res));
}

void app::impl::send_response_file(request req, response res) {
  response::file_info &fi = res.m_file_info;

  if (fi.m_is_view) {
    fi.m_path = join_paths(m_views_dir, fi.m_path).string();
  }

  uv_fs_t *open_req = new uv_fs_t;
  const char *path = fi.m_path.c_str();
  open_req->data = new send_file_ctx(-1, (uv_stream_t *)req.m_remote, std::move(req), std::move(res));
  uv_fs_open(uv_default_loop(), open_req, path, O_RDONLY, 0, app::impl::on_open_file);
}

void app::impl::on_open_file(uv_fs_t *open_req) {
  auto ctx = (send_file_ctx *)open_req->data;
  response::file_info &fi = ctx->m_res.value().m_file_info;

  if (open_req->result < 0) {
    debug::error("Fail to open file: %s, %s", open_req->path, uv_strerror(open_req->result));

    response res = response::ok(status::NOT_FOUND);
    if (fi.m_is_view) {
      res = response::ok(status::INTERNAL_SERVER_ERROR);
    }
    res.set_header("Content-Len", std::to_string(res.m_body.length()));
    app::impl::send_response_head(ctx->m_req.value(), res);
    app::impl::send_response_body(std::move(ctx->m_req.value()), std::move(res));

    // clean up
    delete ctx;
  } else {
    ctx->m_res.value().set_header("Transfer-Encoding", "chunked");
    send_response_head(ctx->m_req.value(), ctx->m_res.value());

    // discard the request and response objects since they are no longer needed
    ctx->m_req.reset();
    ctx->m_res.reset();

    ctx->m_file_fd = open_req->result;
    uv_fs_t *read_req = new uv_fs_t;
    read_req->data = ctx;
    uv_buf_t read_buf = uv_buf_init(ctx->m_chunk, sizeof(ctx->m_chunk));
    uv_fs_read(uv_default_loop(), read_req, ctx->m_file_fd, &read_buf, 1, -1, app::impl::on_read_file_chunk);
  }

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
      debug::error("Fail to read file chunk, %s", uv_strerror(read_req->result));
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
    debug::error("Fail to write response chunk, %s", uv_strerror(status));
  }
  if (req->data) delete[] (char *)req->data;
  delete req;
}

void app::impl::on_write_and_close(uv_write_t *req, int status) {
  auto remote = (uv_handle_t *)req->handle;
  app::impl::on_write_buf(req, status);
  uv_close(remote, app::impl::on_close_conn);
}

void app::impl::on_close_conn(uv_handle_t *client) {
  delete client;
}

} // namespace fc
