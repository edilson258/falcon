#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <sys/param.h>
#include <uv.h>
#include <uv/unix.h>
#include <vector>

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
  root_router m_router;
  http_parser m_http_parser;

  // The 'm_loop->data' field always holds a pointer to an object of this struct
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

  void add_route(method, const std::string, path_handler, const std::vector<path_handler> &);

  // uv callbacks
  static void on_connection(uv_stream_t *server, int status);
  static void on_alloc_buf(uv_handle_t *client, size_t size, uv_buf_t *buf);
  static void on_read_buf(uv_stream_t *client, long nread, const uv_buf_t *buf);
  static void on_write_response(uv_write_t *req, int status);
  static void on_close_conn(uv_handle_t *client);
};

app::app() : m_pimpl(new app::impl()) {}
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
  call_back(host + ":" + port);
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

void app::impl::on_alloc_buf(uv_handle_t *client, size_t size, uv_buf_t *buf) {
  buf->len = size;
  buf->base = new char[size];
}

void app::impl::on_read_buf(uv_stream_t *client, long nread, const uv_buf_t *buf) {
  uv_read_stop(client);
  if (nread < 0) {
    if (nread != UV_EOF)
      std::cerr << "[FALCON ERROR]: Failed to read remote socket, " << uv_strerror(nread) << std::endl;
    delete[] buf->base;
    uv_close((uv_handle_t *)client, app::impl::on_close_conn);
    return;
  }
  ((app::impl *)client->loop->data)->parse_http_request(request_factory((void *)client, std::string_view(buf->base, strnlen(buf->base, MAX_REQ_LEN))));
}

void app::impl::parse_http_request(request req) {
  enum llhttp_errno err = m_http_parser.parse(&req);
  if (HPE_OK != err && HPE_INVALID_METHOD != err) {
    // send 'bad request' response
    std::cerr << "[FALCON ERROR]: Faild to parse request, " << llhttp_errno_name(err) << std::endl;
    return;
  }
  match_request_to_handler(std::move(req));
}

// #include <iostream>
// #include <string>
// #include <vector>
// #include <sstream>
// #include <filesystem>
// #include <stdexcept>

// namespace fs = std::filesystem;

// std::string normalize_path(const std::string& base_dir, const std::string& input_path) {
//     fs::path base(base_dir);
//     fs::path requested_path = fs::path(input_path).lexically_normal();

//     // Resolve the absolute path by combining base_dir and requested_path
//     fs::path full_path = fs::canonical(base / requested_path);

//     // Check if the file path is still inside the base directory
//     if (full_path.string().find(base.string()) != 0) {
//         throw std::invalid_argument("Access outside of allowed directory is not permitted.");
//     }

//     return full_path.string();  // Return the normalized, absolute path
// }

// int main() {
//     try {
//         std::string base_dir = "public";  // The allowed directory for static files
//         std::string requested_path = "../style.css";  // Example input path (malicious)

//         std::string safe_path = normalize_path(base_dir, requested_path);
//         std::cout << "Safe, normalized path: " << safe_path << std::endl;

//     } catch (const std::invalid_argument& e) {
//         std::cerr << "Error: " << e.what() << std::endl;
//     }

//     return 0;
// }

namespace fs = std::filesystem;

void on_open_static_file(uv_fs_t *req) {
  if (req->file < 0) {
    // send 404
  }
  uv_fs_t *fs_req = new uv_fs_t;
  fs_req->data = req->data;
  printf("Stat %d %zu\n", req->file, req->statbuf.st_size);
}

void try_serve_static_file(request req) {
  fs::path base = fs::path(FC_PUBLIC_DIR);
  fs::path full_path = base.concat(req.get_path()).lexically_normal().make_preferred();
  if (full_path.string().find(base.string()) != 0) {
    // probably is a path traversal attack
    // send 404
  }

  const char *path = strndup(full_path.c_str(), MAXPATHLEN);
  printf("Path: %s\n", path);
  uv_fs_t *fs_req = new uv_fs_t;
  uv_tcp_t *remote = (uv_tcp_t *)req.get_remote();
  fs_req->data = remote;
  uv_fs_open(remote->loop, fs_req, path, UV_FS_O_RDONLY, S_IRUSR, on_open_static_file);
}

void app::impl::match_request_to_handler(request req) {
  if (m_router.match(req)) {
    auto res = req.next();
    return send_response(std::move(req), std::move(res));
  }
  try_serve_static_file(req);
}

void app::impl::send_response(request req, response res) {
  uv_buf_t *write_buf = new uv_buf_t;
  *write_buf = uv_buf_init((char *)res.to_string().c_str(), res.to_string().length());
  uv_write_t *write_req = new uv_write_t;
  write_req->data = (void *)req.get_raw().data();
  uv_write(write_req, (uv_stream_t *)req.get_remote(), write_buf, 1, app::impl::on_write_response);
}

void app::impl::add_route(method method, const std::string path, path_handler handler, const std::vector<path_handler> &midwares) {
  m_router.add(method, path, handler, midwares);
}

void app::impl::on_write_response(uv_write_t *req, int status) {
  delete[] (char *)req->data;
  delete req->bufs;
  delete req;
  uv_close((uv_handle_t *)req->handle, app::impl::on_close_conn);
}

void app::impl::on_close_conn(uv_handle_t *client) {
  delete client;
}

} // namespace fc
