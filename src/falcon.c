#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JACK_IMPLEMENTATION
#include <jack.h>

#include <llhttp.h>
#include <uv.h>

#include <falcon.h>
#include <falcon/consts.h>
#include <falcon/errn.h>
#include <falcon/http.h>
#include <falcon/router.h>
#include <falcon/stringview.h>
#include <falcon/templates.h>

struct fc_app
{
  fc_router *router;

  uv_loop_t *main_loop;
  uv_tcp_t server_sock;
  struct sockaddr_in server_addr;

  llhttp_t http_parser;
  llhttp_settings_t http_parser_settings;
};

struct fc_app *self;

// uv IO callbacks
void on_write(uv_write_t *req, int status);
void on_connection(uv_stream_t *server, int status);
void on_alloc_req_buf(uv_handle_t *client, size_t size, uv_buf_t *buf);
void on_read_request(uv_stream_t *client, long nread, const uv_buf_t *buf);
void on_close_connection(uv_handle_t *client);

// llhttp parser
void http_parse_request(char *raw_buffer, size_t buffer_size, fc_request_t *request);
int http_parser_on_url(llhttp_t *p, const char *at, size_t len);
int http_parser_on_method(llhttp_t *p, const char *at, size_t len);
int http_parser_on_body(llhttp_t *p, const char *at, size_t len);
int http_parser_on_header_field(llhttp_t *p, const char *at, size_t len);
int http_parser_on_header_value(llhttp_t *p, const char *at, size_t len);

/**
 * INTERNAL API
 */
void fc__app_init(char *host, unsigned port);
void fc__match_request_to_handler(fc_request_t *request);
fc_errno fc__add_route(fc_router *router, char *path, fc_route_handler_fn handler, fc_http_method method, const fc_schema_t *schema);

/**
 * RESPONSE HANDLERS
 */

void send_404_response(fc_request_t *request);
void send_bad_req_response(fc_request_t *request);
void send_json_response(fc_response_t *res, char *pload, size_t pload_len);
void send_html_response(uv_handle_t *handler, fc_http_status status, char *title, char *message);
void send_http_response(uv_handle_t *handler, fc_http_status status, char *cont_type, char *body, size_t body_len);

/**
 * EXTERANL API
 */

fc_app *fc_app_new()
{
  self = malloc(sizeof(struct fc_app));
  self->router = NULL;
  return self;
}

fc_errno fc_use_router(fc_app *app, fc_router *router)
{
  app->router = router;
  return FC_ERR_OK;
}

void fc_get(fc_router *router, char *path, fc_route_handler_fn handler, const fc_schema_t *schema)
{
  fc_errno err = fc__add_route(router, path, handler, FC_HTTP_GET, schema);
  if (FC_ERR_OK != err)
  {
    fprintf(stderr, "[FALCON ERROR]: Failed to add path (%s) to the router\n", path);
    exit(1);
  }
}

void fc_post(fc_router *router, char *path, fc_route_handler_fn handler, const fc_schema_t *schema)
{
  fc_errno err = fc__add_route(router, path, handler, FC_HTTP_POST, schema);
  if (FC_ERR_OK != err)
  {
    fprintf(stderr, "[FALCON ERROR]: Failed to add path (%s) to the router\n", path);
    exit(1);
  }
}

void fc_put(fc_router *router, char *path, fc_route_handler_fn handler, const fc_schema_t *schema)
{
  fc_errno err = fc__add_route(router, path, handler, FC_HTTP_PUT, schema);
  if (FC_ERR_OK != err)
  {
    fprintf(stderr, "[FALCON ERROR]: Failed to add path (%s) to the router\n", path);
    exit(1);
  }
}

void fc_patch(fc_router *router, char *path, fc_route_handler_fn handler, const fc_schema_t *schema)
{
  fc_errno err = fc__add_route(router, path, handler, FC_HTTP_PATCH, schema);
  if (FC_ERR_OK != err)
  {
    fprintf(stderr, "[FALCON ERROR]: Failed to add path (%s) to the router\n", path);
    exit(1);
  }
}

void fc_delete(fc_router *router, char *path, fc_route_handler_fn handler, const fc_schema_t *schema)
{
  fc_errno err = fc__add_route(router, path, handler, FC_HTTP_DELETE, schema);
  if (FC_ERR_OK != err)
  {
    fprintf(stderr, "[FALCON ERROR]: Failed to add path (%s) to the router\n", path);
    exit(1);
  }
}

void fc_trace(fc_router *router, char *path, fc_route_handler_fn handler, const fc_schema_t *schema)
{
  fc_errno err = fc__add_route(router, path, handler, FC_HTTP_TRACE, schema);
  if (FC_ERR_OK != err)
  {
    fprintf(stderr, "[FALCON ERROR]: Failed to add path (%s) to the router\n", path);
    exit(1);
  }
}

void fc_connect(fc_router *router, char *path, fc_route_handler_fn handler, const fc_schema_t *schema)
{
  fc_errno err = fc__add_route(router, path, handler, FC_HTTP_CONNECT, schema);
  if (FC_ERR_OK != err)
  {
    fprintf(stderr, "[FALCON ERROR]: Failed to add path (%s) to the router\n", path);
    exit(1);
  }
}

void fc_options(fc_router *router, char *path, fc_route_handler_fn handler, const fc_schema_t *schema)
{
  fc_errno err = fc__add_route(router, path, handler, FC_HTTP_OPTIONS, schema);
  if (FC_ERR_OK != err)
  {
    fprintf(stderr, "[FALCON ERROR]: Failed to add path (%s) to the router\n", path);
    exit(1);
  }
}

void fc_head(fc_router *router, char *path, fc_route_handler_fn handler, const fc_schema_t *schema)
{
  fc_errno err = fc__add_route(router, path, handler, FC_HTTP_HEAD, schema);
  if (FC_ERR_OK != err)
  {
    fprintf(stderr, "[FALCON ERROR]: Failed to add path (%s) to the router\n", path);
    exit(1);
  }
}

void fc_res_ok(fc_response_t *res)
{
  char *title = "OK";
  char *message = "OK";
  send_html_response((uv_handle_t *)res->handler, res->status, title, message);
}

void fc_res_json(fc_response_t *res, jjson_t *json)
{
  char *body;
  enum jjson_error err = jjson_stringify(json, 2, &body);
  assert(err == JJE_OK);
  size_t body_len = strlen(body);
  send_json_response(res, body, body_len);
  // TODO: free body
}

fc_errno fc_res_sendfile(fc_response_t *res, const char *path)
{
  // uv_fs_sendfile(uv_loop_t *loop, uv_fs_t *req, uv_file out_fd, uv_file in_fd, int64_t in_offset, size_t length, uv_fs_cb cb);
  return FC_ERR_OK;
}

void fc_res_set_status(fc_response_t *res, fc_http_status status)
{
  res->status = status;
}

fc_errno fc__sv_kv_array_get(fc__sv_kv_array *arr, const char *name, char **out, size_t *out_len)
{
  for (int i = 0; i < arr->count; ++i)
  {
    if (strncasecmp(name, arr->items[i].key.ptr, arr->items[i].key.len) == 0)
    {
      *out = (char *)arr->items[i].value.ptr;
      *out_len = arr->items[i].value.len;
      return FC_ERR_OK;
    }
  }
  return FC_ERR_ENTRY_NOT_FOUND;
}

fc_errno fc_req_get_param(fc_request_t *req, const char *name, char **out, size_t *out_len)
{
  return fc__sv_kv_array_get(&req->params, name, out, out_len);
}

fc_errno fc_req_get_header(fc_request_t *req, const char *name, char **out, size_t *out_len)
{
  return fc__sv_kv_array_get(&req->headers, name, out, out_len);
}

fc_errno fc_req_get_cookie(fc_request_t *req, const char *name, char **out, size_t *out_len)
{
  if (!req->cookies.parsed)
  {
    fc_errno err = fc__req_parse_cookies(req);
    if (FC_ERR_OK != err)
    {
      return err;
    }
  }

  return fc__sv_kv_array_get(&req->cookies.cookies, name, out, out_len);
}

bool match_fc_to_jjson_type(fc_data_t fc_t, jjson_type jj_t)
{
  switch (fc_t)
  {
  case FC_ANY_T:
    return true;
  case FC_INTEGER_T:
  case FC_FLOAT_T:
    return JSON_NUMBER == jj_t;
  case FC_STRING_T:
    return JSON_STRING == jj_t;
  case FC_JSON_T:
    return JSON_OBJECT == jj_t;
  case FC_ARRAY_T:
    return JSON_ARRAY == jj_t;
  case FC_CHAR_T:
    return false;
  }
}

fc_errno fc_req_bind_json(fc_request_t *req, jjson_t *json, const fc_schema_t *schema)
{
  enum jjson_error err = jjson_parse(json, req->raw_body.ptr, req->raw_body.len);
  if (JJE_OK != err)
  {
    return FC_ERR_INVALID_JSON;
  }

  if (schema)
  {
    for (size_t i = 0; i < schema->nfields; ++i)
    {
      jjson_value *json_val = NULL;
      jjson_get(json, schema->fields[i].name, &json_val);
      if (!json_val || !match_fc_to_jjson_type(schema->fields[i].type, json_val->type))
      {
        return FC_ERR_ENTRY_NOT_FOUND;
      }
    }
  }

  return FC_ERR_OK;
}

int fc_listen(fc_app *app, char *host, unsigned int port, fc_on_listen cb)
{
  fc__app_init(host, port);

  int result = uv_tcp_bind(&app->server_sock, (const struct sockaddr *)&app->server_addr, 0);
  if (result)
  {
    fprintf(stderr, "[FALCON ERROR]: Failed to bind at %s:%u, %s\n", host, port, uv_strerror(result));
    return -1;
  }

  result = uv_listen((uv_stream_t *)&app->server_sock, FC__SERVER_BACKLOG, on_connection);
  if (result)
  {
    fprintf(stderr, "[FALCON ERROR]: Failed to listen at %s:%u, %s\n", host, port, uv_strerror(result));
    return -1;
  }

  if (cb)
  {
    cb();
  }

  return uv_run(app->main_loop, UV_RUN_DEFAULT);
}

/**
 * INTERNAL API
 */

void fc__app_init(char *host, unsigned port)
{
  self->main_loop = uv_default_loop();
  uv_tcp_init(self->main_loop, &self->server_sock);
  // TODO: validate host and port
  uv_ip4_addr(host, port, &self->server_addr);

  // init http parser
  llhttp_settings_init(&self->http_parser_settings);
  self->http_parser_settings.on_url = http_parser_on_url;
  self->http_parser_settings.on_method = http_parser_on_method;
  self->http_parser_settings.on_body = http_parser_on_body;
  self->http_parser_settings.on_header_field = http_parser_on_header_field;
  self->http_parser_settings.on_header_value = http_parser_on_header_value;
  llhttp_init(&self->http_parser, HTTP_REQUEST, &self->http_parser_settings);
}

void on_connection(uv_stream_t *server, int status)
{
  if (status < 0)
  {
    fprintf(stderr, "[FALCON ERROR]: Failed to accept new connection, %s\n", uv_strerror(status));
    return;
  }

  uv_tcp_t *client = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
  uv_tcp_init(server->loop, client);
  client->data = server->data;

  int result = uv_accept(server, (uv_stream_t *)client);
  if (result != 0)
  {
    fprintf(stderr, "[FALCON ERROR]: Failed to accept new connection, %s\n", uv_strerror(result));
    return;
  }

  uv_read_start((uv_stream_t *)client, on_alloc_req_buf, on_read_request);
}

void on_alloc_req_buf(uv_handle_t *handle, size_t size, uv_buf_t *buf)
{
  buf->base = (char *)malloc(sizeof(char) * size);
  buf->len = size;
}

void on_read_request(uv_stream_t *client, long nread, const uv_buf_t *buf)
{
  // TODO: stop reading
  if (nread > 0)
  {
    /* Note: ensure that 'request' does not get popped from the stack while being used */
    fc_request_t request;
    request.handler = (struct uv_handle_t *)client;
    request.raw = (fc_stringview_t){.ptr = buf->base, .len = nread};
    request.params.count = 0;
    request.headers.count = 0;
    request.cookies.parsed = false;
    request.cookies.cookies.count = 0;
    http_parse_request(buf->base, nread, &request);
  }
  else
  {
    uv_close((uv_handle_t *)client, on_close_connection);
  }
}

void http_parse_request(char *raw_buf, size_t buf_sz, fc_request_t *request)
{
  self->http_parser.data = request;
  enum llhttp_errno parse_err = llhttp_execute(&self->http_parser, raw_buf, buf_sz);
  llhttp_reset(&self->http_parser);
  if (parse_err != HPE_OK)
  {
    return send_bad_req_response(request);
  }
  fc__match_request_to_handler(request);
}

int http_parser_on_method(llhttp_t *p, const char *at, size_t len)
{
  fc_request_t *req = (fc_request_t *)p->data;
  if (0 == strncmp("GET", at, 3))
  {
    req->method = FC_HTTP_GET;
  }
  else if (0 == strncmp("POST", at, 4))
  {
    req->method = FC_HTTP_POST;
  }
  else if (0 == strncmp("PUT", at, 3))
  {
    req->method = FC_HTTP_PUT;
  }
  else if (0 == strncmp("DELETE", at, 6))
  {
    req->method = FC_HTTP_DELETE;
  }
  else if (0 == strncmp("HEAD", at, 4))
  {
    req->method = FC_HTTP_HEAD;
  }
  else if (0 == strncmp("OPTIONS", at, 6))
  {
    req->method = FC_HTTP_OPTIONS;
  }
  else if (0 == strncmp("TRACE", at, 5))
  {
    req->method = FC_HTTP_TRACE;
  }
  else if (0 == strncmp("CONNECT", at, 7))
  {
    req->method = FC_HTTP_CONNECT;
  }
  else if (0 == strncmp("PATCH", at, 5))
  {
    req->method = FC_HTTP_PATCH;
  }
  else
  {
    return HPE_INVALID_METHOD;
  }
  return HPE_OK;
}

int http_parser_on_body(llhttp_t *p, const char *at, size_t len)
{
  fc_request_t *req = (fc_request_t *)p->data;
  req->raw_body = (fc_stringview_t){.ptr = at, .len = len};
  return HPE_OK;
}

int http_parser_on_url(llhttp_t *p, const char *at, size_t len)
{
  fc_request_t *req = (fc_request_t *)p->data;
  req->path = (fc_stringview_t){.ptr = at, .len = len};
  return HPE_OK;
}

int http_parser_on_header_field(llhttp_t *p, const char *at, size_t len)
{
  fc_request_t *req = (fc_request_t *)p->data;
  if (req->headers.count >= FC__SV_KV_ARRAY_LEN)
  {
    return FC_ERR_LIMIT_EXCEEDED;
  }
  req->headers.items[req->headers.count].key = (fc_stringview_t){.ptr = at, .len = len};
  return HPE_OK;
}

int http_parser_on_header_value(llhttp_t *p, const char *at, size_t len)
{
  fc_request_t *req = (fc_request_t *)p->data;
  req->headers.items[req->headers.count++].value = (fc_stringview_t){.ptr = at, .len = len};
  return HPE_OK;
}

void fc__match_request_to_handler(fc_request_t *request)
{
  char *path;
  assert(FC_ERR_OK == fc_stringview_get(&path, &request->path));

  bool is_body_parsed = false;

  if (!self->router)
  {
    send_404_response(request);
    goto defer;
  }

  fc__route_handler *handler;
  if (FC_ERR_OK == fc__router_match_req(self->router, request, path, &handler))
  {
    /* Note: ensure that 'response' does not get popped from the stack while being used */
    fc_response_t response;
    response.status = FC_STATUS_OK;
    response.handler = request->handler;

    if (handler->schema)
    {
      if (!request->raw_body.ptr || !request->raw_body.ptr[0])
      {
        /* TODO: send unprocessable entity response */
        send_bad_req_response(request);
        goto defer;
      }

      is_body_parsed = true;

      jjson_t json;
      jjson_init(&json);

      fc_errno err = fc_req_bind_json(request, &json, handler->schema);
      if (FC_ERR_OK != err)
      {
        /* TODO: send unprocessable entity response */
        send_bad_req_response(request);
        goto defer;
      }

      request->body = (void *)&json;
    }

    handler->handle(request, &response);
  }
  else
  {
    send_404_response(request);
  }

defer:
  free(path);
  free((void *)request->raw.ptr);
  if (is_body_parsed)
  {
    // TODO: put this in the workers queue (uv thread pool)
    jjson_deinit(request->body);
  }
}

void on_write(uv_write_t *req, int status)
{
  free(req->bufs);
  free(req);
  uv_close((uv_handle_t *)req->handle, on_close_connection);
}

void on_close_connection(uv_handle_t *client)
{
  free(client);
}

fc_errno fc__add_route(fc_router *router, char *path, fc_route_handler_fn handler, fc_http_method method, const fc_schema_t *schema)
{
  char *p;
  fc_errno err = fc_string_clone(&p, path, strnlen(path, FC__STRING_MAX_LEN));
  if (FC_ERR_OK != err)
  {
    return err;
  }
  return fc__router_add_route(router, method, p, handler, schema);
}

/**
 * RESPONSE HANDLERS
 */

void send_404_response(fc_request_t *request)
{
  char *path;
  assert(FC_ERR_OK == fc_stringview_get(&path, &request->path));

  char *title = "Not found";
  char *method_str = fc__http_method_str(request->method);
  size_t message_len = snprintf(NULL, 0, "Cannot %s %s", method_str, path) + 1;
  char message[message_len];
  snprintf(message, message_len, "Cannot %s %s", method_str, path);
  send_html_response((uv_handle_t *)request->handler, FC_STATUS_NOT_FOUND, title, message);

  free(path);
}

void send_bad_req_response(fc_request_t *request)
{
  char *title = "Bad request";
  char *message = "Bad request";
  fc_http_status status = FC_STATUS_BAD_REQUEST;
  send_html_response((uv_handle_t *)request->handler, status, title, message);
}

void send_html_response(uv_handle_t *handler, fc_http_status status, char *title, char *message)
{
  size_t body_buf_sz = snprintf(NULL, 0, HTML_TEMPLATE, title, message) + 1;
  char body_buf[body_buf_sz];
  snprintf(body_buf, body_buf_sz, HTML_TEMPLATE, title, message);
  send_http_response(handler, status, CONTENT_TYPE_HTML, body_buf, body_buf_sz);
}

void send_json_response(fc_response_t *res, char *pload, size_t pload_len)
{
  send_http_response((uv_handle_t *)res->handler, res->status, CONTENT_TYPE_JSON, pload, pload_len);
}

void send_http_response(uv_handle_t *handler, fc_http_status status, char *cont_type, char *body, size_t body_len)
{
  char *status_str = fc__http_status_str(status);
  size_t head_len = snprintf(NULL, 0, HTTP_RESPONSE_HEADER_TEMPLATE, status, status_str, cont_type, body_len - 1) + 1;
  char head[head_len];
  snprintf(head, head_len, HTTP_RESPONSE_HEADER_TEMPLATE, status, status_str, cont_type, body_len - 1);

  uv_buf_t *write_bufs = (uv_buf_t *)malloc(sizeof(uv_buf_t) * 2);
  write_bufs[0] = uv_buf_init(head, head_len - 1);
  write_bufs[1] = uv_buf_init(body, body_len - 1);

  uv_write_t *write_req = (uv_write_t *)malloc(sizeof(uv_write_t));
  uv_write(write_req, (uv_stream_t *)handler, write_bufs, 2, on_write);
}
