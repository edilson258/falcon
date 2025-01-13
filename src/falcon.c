#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JACK_IMPLEMENTATION
#include <jack.h>
#include <llhttp.h>
#include <uv.h>

#include <falcon.h>

char CONTENT_TYPE_PLAIN[] = "text/plain";
char CONTENT_TYPE_HTML[] = "text/html";
char CONTENT_TYPE_JSON[] = "application/json";

char OK_RESPONSE[] = "HTTP/1.1 200 OK\r\n"
                     "Server: Falcon\r\n"
                     "Content-Type: text/plain\r\n"
                     "Content-Length: 2\r\n"
                     "\r\n"
                     "Ok";

char JSON_RESPONSE_TEMPLATE[] = "HTTP/1.1 200 OK\r\n"
                                "Server: Falcon\r\n"
                                "Content-Type: application/json\r\n"
                                "Content-Length: %zu\r\n"
                                "\r\n"
                                "%s";

char RESPONSE_TEMPLATE[] = "HTTP/1.1 %d %s\r\n"
                           "Server: Falcon\r\n"
                           "Content-Type: %s\r\n"
                           "Content-Length: %zu\r\n"
                           "\r\n"
                           "%s";

char HTTP_RESPONSE_HEADER_TEMPLATE[] = "HTTP/1.1 %d %s\r\n"
                                       "Server: Falcon\r\n"
                                       "Content-Type: %s\r\n"
                                       "Content-Length: %zu\r\n"
                                       "\r\n";

char HTML_TEMPLATE[] = "<!DOCTYPE html>\n"
                       "<html lang=\"en\">\n"
                       "<head>\n"
                       "  <meta charset=\"UTF-8\">\n"
                       "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                       "  <title>%s</title>\n"
                       "</head>\n"
                       "<body>\n"
                       "  %s\n"
                       "</body>\n"
                       "</html>\n";

#define FHTTP_METHOD_STR(method)     \
  ((method) == FHTTP_GET    ? "GET"  \
   : (method) == FHTTP_POST ? "POST" \
                            : "Unkown HTTP method")

#define FHTTP_STATUS_STR(status)                        \
  ((status) == FHTTP_STATUS_OK          ? "OK"          \
   : (status) == FHTTP_STATUS_BAD_REQ   ? "Bad request" \
   : (status) == FHTTP_STATUS_NOT_FOUND ? "Not found"   \
                                        : "Unkown HTTP status code")

// Globals
uv_loop_t *main_loop_glob;
uv_tcp_t server_sock_glob;
struct sockaddr_in server_addr_glob;
fRoute *routes_glob = NULL;

// uv IO callbacks
void on_write(uv_write_t *req, int status);
void on_connection(uv_stream_t *server, int status);
void on_alloc_req_buf(uv_handle_t *client, size_t size, uv_buf_t *buf);
void on_read_request(uv_stream_t *client, long nread, const uv_buf_t *buf);
void on_close_connection(uv_handle_t *client);

// llhttp parser
void on_parse_request(char *raw_buffer, size_t buffer_size, frequest_t *request);
int on_url(llhttp_t *p, const char *at, size_t len);
int on_method(llhttp_t *p, const char *at, size_t len);
int on_body(llhttp_t *p, const char *at, size_t len);

/**
 * INTERNAL API
 */
int init_server(char *host, unsigned port);
void on_match_request_handler(frequest_t *request);
char *make_route_id(char *path, fhttp_method method);
void create_route(falcon_t *app, char *path, froute_handler_t handler, fhttp_method method, fschema_t *schema);

/**
 * RESPONSE HANDLERS
 */

void send_404_response(frequest_t *request);
void send_bad_req_response(frequest_t *request);
void send_json_response(fresponse_t *res, char *pload, size_t pload_len);
void send_html_response(uv_handle_t *handler, fhttp_status status, char *title, char *message);
void send_http_response(uv_handle_t *handler, fhttp_status status, char *cont_type, char *body, size_t body_len);

/**
 * EXTERANL API
 */

void fget(falcon_t *app, char *path, froute_handler_t handler)
{
  create_route(app, path, handler, FHTTP_GET, 0);
}

void fpost(falcon_t *app, char *path, froute_handler_t handler, fschema_t *schema)
{
  create_route(app, path, handler, FHTTP_POST, schema);
}

void fres_ok(fresponse_t *response)
{
  char *title = "OK";
  char *message = "OK";
  fhttp_status status = FHTTP_STATUS_OK;
  send_html_response(response->handler, status, title, message);
}

void fres_json(fresponse_t *res, jjson_t *json)
{
  char *body;
  enum jjson_error err = jjson_stringify(json, 2, &body);
  assert(err == JJE_OK);
  size_t body_len = strlen(body);
  send_json_response(res, body, body_len);
}

int flisten(falcon_t *app, char *host, unsigned int port, fon_listen_t cb)
{
  int result = 0;
  init_server(host, port);

  result = uv_tcp_bind(&server_sock_glob, (const struct sockaddr *)&server_addr_glob, 0);
  if (result)
  {
    fprintf(stderr, "Bind failed, %s\n", uv_strerror(result));
    return -1;
  }

  result = uv_listen((uv_stream_t *)&server_sock_glob, 1024, on_connection);
  if (result)
  {
    fprintf(stderr, "Listen failed, %s\n", uv_strerror(result));
    return -1;
  }

  if (cb)
    cb();

  return uv_run(main_loop_glob, UV_RUN_DEFAULT);
}

/**
 * INTERNAL API
 */

int init_server(char *host, unsigned port)
{
  main_loop_glob = uv_default_loop();
  uv_tcp_init(main_loop_glob, &server_sock_glob);
  // TODO: validate host and port
  uv_ip4_addr(host, port, &server_addr_glob);
  return 0;
}

/**
 * This function is called when server has an incomming request
 *
 */
void on_connection(uv_stream_t *server, int status)
{
  if (status < 0)
  {
    fprintf(stderr, "Failed on new connection, %s\n", uv_strerror(status));
    return;
  }

  uv_tcp_t *client = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
  uv_tcp_init(server->loop, client);
  client->data = server->data;

  int result = uv_accept(server, (uv_stream_t *)client);
  if (result != 0)
  {
    fprintf(stderr, "Accept failed, %s\n", uv_strerror(result));
    return;
  }

  uv_read_start((uv_stream_t *)client, on_alloc_req_buf, on_read_request);
}

void on_alloc_req_buf(uv_handle_t *handle, size_t size, uv_buf_t *buf)
{
  buf->base = (char *)malloc(sizeof(char) * size);
  buf->len = size;
}

/**
 * A callback run after reading the client socket has completed
 *
 */
void on_read_request(uv_stream_t *client, long nread, const uv_buf_t *buf)
{
  if (nread > 0)
  {
    frequest_t *req = (frequest_t *)malloc(sizeof(frequest_t));
    req->handler = (uv_handle_t *)client;
    on_parse_request(buf->base, nread, req);
  }
  else
  {
    uv_close((uv_handle_t *)client, on_close_connection);
  }
}

/**
 * Parse raw buffer populating the request instance with relevant data
 *
 */
void on_parse_request(char *raw_buf, size_t buf_sz, frequest_t *request)
{
  // TODO: reuse same parser
  llhttp_t parser;
  llhttp_settings_t settings;
  llhttp_settings_init(&settings);

  settings.on_url = on_url;
  settings.on_method = on_method;
  settings.on_body = on_body;

  llhttp_init(&parser, HTTP_REQUEST, &settings);
  parser.data = request;

  enum llhttp_errno parse_err = llhttp_execute(&parser, raw_buf, buf_sz);

  // At this point the request has been parsed and the raw buffer is no longer needed
  free(raw_buf);

  if (parse_err != HPE_OK)
    return send_bad_req_response(request);

  on_match_request_handler(request);
}

int on_method(llhttp_t *p, const char *at, size_t len)
{
  frequest_t *req = (frequest_t *)p->data;
  if (0 == strncmp("GET", at, len))
  {
    req->method = FHTTP_GET;
    return HPE_OK;
  }
  else if (0 == strncmp("POST", at, len))
  {
    req->method = FHTTP_POST;
    return HPE_OK;
  }
  return HPE_INVALID_METHOD;
}

int on_body(llhttp_t *p, const char *at, size_t len)
{
  frequest_t *req = (frequest_t *)p->data;
  req->body = malloc(sizeof(char) * (len + 1));
  strncpy((char *)req->body, at, len);
  ((char *)req->body)[len] = '\0';
  return HPE_OK;
}

int on_url(llhttp_t *p, const char *at, size_t len)
{
  frequest_t *req = (frequest_t *)p->data;
  req->path = malloc(sizeof(char) * (len + 1));
  strncpy(req->path, at, len);
  req->path[len] = '\0';
  return HPE_OK;
}

/**
 * Match an already parsed request to a user defined route to be handled
 */
void on_match_request_handler(frequest_t *request)
{
  fRoute *match_route;
  char *id = make_route_id(request->path, request->method);
  HASH_FIND_STR(routes_glob, id, match_route);

  if (!match_route)
  {
    return send_404_response(request);
  }

  if (match_route->schema)
  {
    jjson_t *json = (jjson_t *)malloc(sizeof(jjson_t));
    jjson_init(json);

    enum jjson_error err = jjson_parse(json, request->body);
    if (JJE_OK != err)
      return send_bad_req_response(request);

    for (int i = 0; i < match_route->schema->nfields; ++i)
    {
      const ffield_t *field = &match_route->schema->fields[i];
      jjson_value *match_val;
      err = jjson_get(json, field->name, &match_val);
      if (JJE_OK != err || match_val->type != field->type)
        return send_bad_req_response(request);
    }

    free(request->body);
    request->body = json;
  }

  fresponse_t *response = (fresponse_t *)malloc(sizeof(fresponse_t));
  response->status = FHTTP_STATUS_OK;
  response->handler = request->handler;
  match_route->handler(request, response);
}

void on_write(uv_write_t *req, int status)
{
  for (int i = 0; i < req->nbufs; ++i)
  {
    // free(req->bufs[0].base);
  }

  free(req->bufs);
  free(req);

  uv_close((uv_handle_t *)req->handle, on_close_connection);
}

/**
 * A callback run after closing the client socket has completed
 *
 */
void on_close_connection(uv_handle_t *client)
{
  free(client);
}

void create_route(falcon_t *app, char *path, froute_handler_t handler, fhttp_method method, fschema_t *schema)
{
  fRoute *route = (fRoute *)malloc(sizeof(fRoute));
  route->id = make_route_id(path, method);
  route->path = path;
  route->method = method;
  route->handler = handler;
  route->schema = schema;
  HASH_ADD_STR(routes_glob, id, route);
}

char *make_route_id(char *path, fhttp_method method)
{
  size_t buf_sz = snprintf(NULL, 0, "%s-%d", path, method) + 1;
  char *buf = malloc(sizeof(char) * buf_sz);
  snprintf(buf, buf_sz, "%s-%d", path, method);
  return buf;
}

/**
 * RESPONSE HANDLERS
 */

void send_http_response(uv_handle_t *handler, fhttp_status status, char *cont_type, char *body, size_t body_len)
{
  size_t head_len = snprintf(NULL, 0, HTTP_RESPONSE_HEADER_TEMPLATE, status, FHTTP_STATUS_STR(status), cont_type, body_len) + 1;
  char head[head_len];
  snprintf(head, head_len, HTTP_RESPONSE_HEADER_TEMPLATE, status, FHTTP_STATUS_STR(status), cont_type, body_len);

  uv_buf_t *write_bufs = (uv_buf_t *)malloc(sizeof(uv_buf_t) * 2);
  write_bufs[0] = uv_buf_init(head, head_len - 1);
  write_bufs[1] = uv_buf_init(body, body_len - 1);

  uv_write_t *write_req = (uv_write_t *)malloc(sizeof(uv_write_t));
  uv_write(write_req, (uv_stream_t *)handler, write_bufs, 2, on_write);
}

void send_html_response(uv_handle_t *handler, fhttp_status status, char *title, char *message)
{
  size_t body_buf_sz = snprintf(NULL, 0, HTML_TEMPLATE, title, message) + 1;
  char body_buf[body_buf_sz];
  snprintf(body_buf, body_buf_sz, HTML_TEMPLATE, title, message);

  send_http_response(handler, status, CONTENT_TYPE_HTML, body_buf, body_buf_sz);
}

void send_404_response(frequest_t *request)
{
  char *title = "Not found";
  size_t message_len = snprintf(NULL, 0, "Cannot %s %s", FHTTP_METHOD_STR(request->method), request->path) + 1;
  char message[message_len];
  snprintf(message, message_len, "Cannot %s %s", FHTTP_METHOD_STR(request->method), request->path);
  fhttp_status status = FHTTP_STATUS_NOT_FOUND;
  send_html_response(request->handler, status, title, message);
}

void send_bad_req_response(frequest_t *request)
{
  char *title = "Bad request";
  char *message = "Bad request";
  fhttp_status status = FHTTP_STATUS_BAD_REQ;
  send_html_response(request->handler, status, title, message);
}

void send_json_response(fresponse_t *res, char *pload, size_t pload_len)
{
  send_http_response(res->handler, res->status, CONTENT_TYPE_JSON, pload, pload_len);
}
