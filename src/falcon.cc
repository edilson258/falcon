#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <string>

#include "const.h"
#include "http.h"
#include "llhttp.h"
#include "request.h"
#include "response.h"
#include "router.h"
#include "signals.h"
#include "spdlog/spdlog.h"
#include "utils.h"

#define FC_BACKLOG (128)

namespace fs = std::filesystem;

namespace fc {

struct app::impl {
  root_router m_router;
  http_parser m_http_parser;

  uv_loop_t *m_loop;
  uv_tcp_t m_host_sock{};
  sockaddr_in m_addr{};

  fs::path m_views;
  fs::path m_assets;

  impl() : m_loop(uv_default_loop()) {
    m_loop->data = this;
    uv_tcp_init(m_loop, &m_host_sock);

    m_views = fs::absolute("views/");
    m_assets = fs::absolute("public/");
  }

  void parse_http_request(request);
  void match_request_to_handler(request) const;

  // response handlers
  void try_serve_static_file(request) const;
  void send_response(request, response) const;
  void send_response_file(request, response) const;
  static void send_response_head(const request &, const response &);
  static void send_response_body(request, response);
  static void send_response_file_error(request, response);

  // uv callbacks
  static void on_close_conn(uv_handle_t *client);
  static void on_open_file(uv_fs_t *);
  static void on_stat_file(uv_fs_t *);
  static void on_read_file_chunk(uv_fs_t *);
  static void on_connection(uv_stream_t *host, int status);
  static void on_write_buf(uv_write_t *req, int status);
  static void on_write_and_close(uv_write_t *req, int status);
  static void on_alloc_req_buf(uv_handle_t *client, size_t len, uv_buf_t *buf);
  static void on_read_req_buf(uv_stream_t *client, long nread, const uv_buf_t *buf);

  void add_route(method, const std::string &, const path_handler &, const std::vector<path_handler> &);
};

struct file_sender {
  int m_fd;
  uv_stream_t *m_remote;
  char m_chunk[64 * 1024]{}; // 64KB

  std::optional<request> m_req;
  std::optional<response> m_res;

  file_sender(const int fd, uv_stream_t *sock, request req, response res)
      : m_fd(fd), m_remote(sock), m_req(std::move(req)), m_res(std::move(res)) {
    reset_chunk();
  };

  ~file_sender() {
    if (m_fd == -1) {
      return;
    }
    uv_fs_t close_req;
    uv_fs_close(uv_default_loop(), &close_req, m_fd, []([[maybe_unused]] uv_fs_t *req_) {});
  }

  void reset_chunk() { std::memset(m_chunk, 0, sizeof(m_chunk)); }
};

app::app() {
  signals::ignore_sigpipe(); // must be called before any uv functions
  m_pimpl = new impl();
}

app::~app() { delete m_pimpl; }

void app::get(const std::string &path_, const path_handler &handler_) const {
  m_pimpl->add_route(method::GET, path_, handler_, {});
}

void app::post(const std::string &path_, const path_handler &handler_) const {
  m_pimpl->add_route(method::POST, path_, handler_, {});
}

void app::put(const std::string &path, const path_handler &handler) const {
  m_pimpl->add_route(method::PUT, path, handler, {});
}

void app::delet(const std::string &path, const path_handler &handler) const {
  m_pimpl->add_route(method::DELETE, path, handler, {});
}

void app::patch(const std::string &path, const path_handler &handler) const {
  m_pimpl->add_route(method::PATCH, path, handler, {});
}

void app::use(const router &router) const {
  for (auto &r : router.m_pimpl->m_routes) {
    m_pimpl->add_route(r.m_method, router.m_pimpl->m_base + r.m_path, r.m_handler, router.m_pimpl->m_middlewares);
  }
}

void app::set_views_dir(const std::string &dir_path) const { m_pimpl->m_views = dir_path; }

void app::set_assets_dir(const std::string &dir_path) const {
  //
  m_pimpl->m_assets = fs::absolute(dir_path);
}

void app::impl::add_route(const method method_, const std::string &path_, const path_handler &handler_,
                          const std::vector<path_handler> &middlewares_) {
  std::string norm_path = path_;
  if (norm_path.empty() || path_.at(0) != '/') {
    norm_path.insert(0, "/");
  }
  m_router.add(method_, norm_path, handler_, middlewares_);
}

int app::listen(const std::string &addr, const std::function<void(const std::string &)> &call_back) const {
  char *err_msg;
  if (0 != m_pimpl->m_router.m_tree.compile(&err_msg)) {
    spdlog::error("Failed to compile route tree, {}", err_msg);
    free(err_msg);
    return -1;
  }
  auto [host, port] = split_address(addr);
  uv_ip4_addr(host.c_str(), std::stoi(port), &m_pimpl->m_addr);
  int result = uv_tcp_bind(&m_pimpl->m_host_sock, reinterpret_cast<const struct sockaddr *>(&m_pimpl->m_addr), 0);
  if (result) {
    spdlog::error("Failed to bind at {}, {}", addr, uv_strerror(result));
    return -1;
  }
  result = uv_listen(reinterpret_cast<uv_stream_t *>(&m_pimpl->m_host_sock), FC_BACKLOG, impl::on_connection);
  if (result) {
    spdlog::error("Failed to listen at {}, {}", addr, uv_strerror(result));
    return -1;
  }
  if (call_back) {
    call_back(host + ":" + port);
  }
  return uv_run(m_pimpl->m_loop, UV_RUN_DEFAULT);
}

void app::impl::on_connection(uv_stream_t *host, const int status) {
  if (status < 0) {
    spdlog::error("Failed to accept new connection, {}", uv_strerror(status));
    return;
  }

  auto *remote = new uv_tcp_t;
  uv_tcp_init(host->loop, remote);

  const int result = uv_accept(host, reinterpret_cast<uv_stream_t *>(remote));
  if (0 != result) {
    spdlog::error("Fail to accept new connection, {}", uv_strerror(result));
  } else {
    uv_read_start(reinterpret_cast<uv_stream_t *>(remote), on_alloc_req_buf, on_read_req_buf);
  }
}

void app::impl::on_alloc_req_buf([[maybe_unused]] uv_handle_t *client, size_t len, uv_buf_t *buf) {
  buf->len = len;
  buf->base = new char[len];
  std::memset(buf->base, 0, len);
}

void app::impl::on_read_req_buf(uv_stream_t *client, long nread, const uv_buf_t *buf) {
  uv_read_stop(client);
  if (nread < 0) {
    if (nread != UV_EOF) {
      spdlog::error("Fail to read remote socket, {}", uv_strerror(static_cast<int>(nread)));
    }
    delete[] buf->base;
    uv_close(reinterpret_cast<uv_handle_t *>(client), on_close_conn);
  } else {
    // TODO: don't store this app instance in loop data
    const auto this_ = static_cast<impl *>(client->loop->data);
    std::unique_ptr<char[]> raw(buf->base);
    auto req = request(new request::impl(client, std::move(raw)));
    this_->parse_http_request(std::move(req));
  }
}

void app::impl::parse_http_request(request req) {
  const llhttp_errno err = m_http_parser.parse(&req);
  if (HPE_OK != err) {
    spdlog::error("Failed to parse http request, {}", llhttp_errno_name(err));
    return send_response(std::move(req), response::ok(status::BAD_REQUEST));
  }
  return match_request_to_handler(std::move(req));
}

void app::impl::match_request_to_handler(request req) const {
  if (m_router.match(req)) {
    response res = req.next();
    send_response(std::move(req), std::move(res));
  } else if (method::GET == req.m_pimpl->m_method) {
    try_serve_static_file(std::move(req));
  } else {
    send_response(std::move(req), response::ok(status::NOT_FOUND));
  }
}

void app::impl::try_serve_static_file(request req) const {
  auto path = join_paths(m_assets, std::string(req.get_path())).string();
  if (path.starts_with(m_assets.string())) {
    response_file file{std::move(path), file_loader::External};
    auto res = response(new response::impl(status::OK, std::move(file)));
    send_response_file(std::move(req), std::move(res));
  } else {
    send_response(std::move(req), response::ok(status::NOT_FOUND));
  }
}

void app::impl::send_response_head(const request &req, const response &res) {
  size_t tmp_len = 0;
  for (const auto &[key, value] : res.m_pimpl->m_headers) {
    tmp_len += key.length() + value.length();
    tmp_len += 1 + 2; // +1 for the separator ':' and +2 for '\r\n'
  }

  const auto tmp_buf = new char[tmp_len + 1];
  char *tmp_buf_ptr = tmp_buf;
  size_t remaining = tmp_len + 1;
  for (const auto &[key, value] : res.m_pimpl->m_headers) {
    const int written = snprintf(tmp_buf_ptr, remaining, "%s:%s\r\n", key.c_str(), value.c_str());
    // TODO: check for 'snprintf' error
    tmp_buf_ptr += written;
    remaining -= static_cast<size_t>(written);
  }

  const auto status = static_cast<int>(res.m_pimpl->m_status);
  const auto status_str = status_to_string(res.m_pimpl->m_status);
  const auto headers_len = snprintf(nullptr, 0, header_cfmt, status, status_str, tmp_buf);
  const auto headers_buf = new char[static_cast<size_t>(headers_len + 1)];
  snprintf(headers_buf, static_cast<size_t>(headers_len) + 1, header_cfmt, status, status_str, tmp_buf);
  // TODO: check for 'snprintf' error
  headers_buf[headers_len] = '\0';

  const auto write_req = new uv_write_t;
  write_req->data = headers_buf;
  const uv_buf_t write_buf = uv_buf_init(headers_buf, static_cast<unsigned int>(headers_len));
  uv_write(write_req, req.m_pimpl->m_remote, &write_buf, 1, on_write_buf);

  delete[] tmp_buf;
}

void app::impl::send_response_body(request req_, response res_) {
  const auto &text = std::get<response_text>(res_.m_pimpl->m_payload);

  const auto body = new char[text.length() + 1];
  strncpy(body, text.c_str(), text.length());
  body[text.length()] = '\0';

  const auto uv_buf = uv_buf_init(body, static_cast<unsigned int>(text.length()));
  const auto write_req = new uv_write_t;
  write_req->data = body;
  uv_write(write_req, req_.m_pimpl->m_remote, &uv_buf, 1, on_write_and_close);
};

void app::impl::send_response(request req, response res) const {
  if (res.m_pimpl->is_file()) {
    send_response_file(std::move(req), std::move(res));
  } else {
    send_response_head(req, res);
    send_response_body(std::move(req), std::move(res));
  }
}

void app::impl::send_response_file(request req, response res) const {
  auto &file = std::get<response_file>(res.m_pimpl->m_payload);

  if (file.m_loader == file_loader::Render) {
    file.m_path = join_paths(m_views, file.m_path).string();
  }

  auto *open_req = new uv_fs_t;
  const char *path = file.m_path.c_str();
  uv_stream_t *remote = req.m_pimpl->m_remote;
  auto *ctx = new file_sender(-1, remote, std::move(req), std::move(res));
  open_req->data = ctx;
  uv_fs_open(uv_default_loop(), open_req, path, O_RDONLY, 0, on_open_file);
}

void app::impl::on_open_file(uv_fs_t *open_req) {
  auto *ctx = static_cast<file_sender *>(open_req->data);

  if (open_req->result < 0) {
    send_response_file_error(std::move(ctx->m_req.value()), std::move(ctx->m_res.value()));
    delete ctx;
  } else {
    ctx->m_fd = static_cast<int>(open_req->result);
    const auto stat_req = new uv_fs_t;
    stat_req->data = ctx;
    uv_fs_fstat(uv_default_loop(), stat_req, static_cast<int>(open_req->result), on_stat_file);
  }

  delete open_req;
}

void app::impl::on_stat_file(uv_fs_t *stat_req) {
  const auto ctx = static_cast<file_sender *>(stat_req->data);

  if (stat_req->result >= 0 && S_ISREG(stat_req->statbuf.st_mode)) {
    ctx->m_res.value().set_header("Transfer-Encoding", "chunked");
    send_response_head(ctx->m_req.value(), ctx->m_res.value());

    ctx->m_req.reset();
    ctx->m_res.reset();

    const auto read_req = new uv_fs_t;
    read_req->data = ctx;
    const uv_buf_t read_buf = uv_buf_init(ctx->m_chunk, sizeof(ctx->m_chunk));
    uv_fs_read(uv_default_loop(), read_req, ctx->m_fd, &read_buf, 1, -1, on_read_file_chunk);
  } else {
    send_response_file_error(std::move(ctx->m_req.value()), std::move(ctx->m_res.value()));
    delete ctx;
  }

  delete stat_req;
}

void app::impl::send_response_file_error(request req_, response res_) {
  const auto &file = std::get<response_file>(res_.m_pimpl->m_payload);

  status status;
  if (file.m_loader == file_loader::External) {
    status = status::NOT_FOUND;
  } else {
    status = status::INTERNAL_SERVER_ERROR;
  }

  auto err_res = response::ok(status);
  send_response_head(req_, err_res);
  send_response_body(std::move(req_), std::move(err_res));
}

void app::impl::on_read_file_chunk(uv_fs_t *read_req) {
  auto *ctx = static_cast<file_sender *>(read_req->data);

  if (read_req->result > 0) {
    const auto nread = static_cast<unsigned int>(read_req->result);
    const auto chunk_len = snprintf(nullptr, 0, "%x\r\n%s\r\n", nread, ctx->m_chunk);
    const auto chunk_buf = new char[static_cast<size_t>(chunk_len) + 1];
    snprintf(chunk_buf, static_cast<size_t>(chunk_len) + 1, "%x\r\n%s\r\n", nread, ctx->m_chunk);
    chunk_buf[chunk_len] = '\0';

    const uv_buf_t write_buf = uv_buf_init(chunk_buf, static_cast<unsigned int>(chunk_len));
    auto *write_req = new uv_write_t;
    write_req->data = chunk_buf;
    uv_write(write_req, ctx->m_remote, &write_buf, 1, on_write_buf);

    // read next chunk
    ctx->reset_chunk();
    auto *next_read_req = new uv_fs_t;
    next_read_req->data = ctx;
    const uv_buf_t next_read_buf = uv_buf_init(ctx->m_chunk, sizeof(ctx->m_chunk));
    uv_fs_read(uv_default_loop(), next_read_req, ctx->m_fd, &next_read_buf, 1, -1, on_read_file_chunk);
  } else {
    if (read_req->result < 0) {
      spdlog::error("Failed to read file chunk, {}", uv_strerror(static_cast<int>(read_req->result)));
    }

    static const auto last_chunk = std::string("0\r\n\r\n");
    const auto last_chunk_buf = uv_buf_init(const_cast<char *>(last_chunk.c_str()), last_chunk.length());
    auto *write_req = new uv_write_t;
    write_req->data = nullptr;
    uv_write(write_req, ctx->m_remote, &last_chunk_buf, 1, on_write_and_close);

    delete ctx;
  }
  delete read_req;
}

void app::impl::on_write_buf(uv_write_t *req, const int status) {
  if (status < 0) {
    spdlog::error("Failed to write on remote, {}", uv_strerror(status));
  }
  if (req->data) {
    delete[] static_cast<char *>(req->data);
  }
  delete req;
}

void app::impl::on_write_and_close(uv_write_t *req, const int status) {
  const auto remote = reinterpret_cast<uv_handle_t *>(req->handle);
  on_write_buf(req, status);
  uv_close(remote, on_close_conn);
}

void app::impl::on_close_conn(uv_handle_t *client) { delete reinterpret_cast<uv_tcp_t *>(client); }

} // namespace fc
