#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JACK_IMPLEMENTATION
#include <jack.h>
#include <llhttp.h>
#include <uthash.h>
#include <uv.h>

#include <falcon.h>

static char CONTENT_TYPE_PLAIN[] = "text/plain";
static char OK_RESPONSE[] = "HTTP/1.1 200 OK\r\n"
                            "Server: Falcon\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: 2\r\n"
                            "\r\n"
                            "Ok";

static char JSON_RESPONSE_TEMPLATE[] = "HTTP/1.1 200 OK\r\n"
                                       "Server: Falcon\r\n"
                                       "Content-Type: application/json\r\n"
                                       "Content-Length: %zu\r\n"
                                       "\r\n"
                                       "%s";
typedef struct
{
  char *id;
  char *path;
  fHttpMethod method;
  fRouteHandler handler;
  fSchema *schema;

  UT_hash_handle hh; // make struct hashable
} Route;

int route_id = 0;
Route *Routes_glob = NULL;

// uv IO callbacks
void on_write(uv_write_t *req, int status);
void on_connection(uv_stream_t *server, int status);
void on_alloc_req_buf(uv_handle_t *client, size_t size, uv_buf_t *buf);
void on_read_request(uv_stream_t *client, long nread, const uv_buf_t *buf);
void on_close_connection(uv_handle_t *client);

// llhttp parser
void parse_request(char *raw_buffer, size_t buffer_size, fReq *request);
int on_url(llhttp_t *p, const char *at, size_t len);
int on_method(llhttp_t *p, const char *at, size_t len);
int on_body(llhttp_t *p, const char *at, size_t len);

char *make_route_id(char *path, fHttpMethod method)
{
  size_t buf_sz = snprintf(NULL, 0, "%s-%d", path, method) + 1;
  char *buf = malloc(sizeof(char) * buf_sz);
  snprintf(buf, buf_sz, "%s-%d", path, method);
  return buf;
}

void fGet(fApp *app, char *path, fRouteHandler handler)
{
  Route *r = (Route *)malloc(sizeof(Route));
  r->id = make_route_id(path, FHTTP_GET);
  r->path = path;
  r->method = FHTTP_GET;
  r->handler = handler;
  r->schema = NULL;
  HASH_ADD_STR(Routes_glob, id, r);
}

void fPost(fApp *app, char *path, fRouteHandler handler, fSchema *schema)
{
  Route *r = (Route *)malloc(sizeof(Route));
  r->id = make_route_id(path, FHTTP_POST);
  r->path = path;
  r->method = FHTTP_POST;
  r->handler = handler;
  r->schema = schema;
  HASH_ADD_STR(Routes_glob, id, r);
}

void fResOk(fRes *response)
{
  uv_buf_t *buf = (uv_buf_t *)malloc(sizeof(uv_buf_t));
  uv_write_t *write_req = (uv_write_t *)malloc(sizeof(uv_write_t));

  buf->base = OK_RESPONSE;
  buf->len = strlen(OK_RESPONSE);

  uv_write(write_req, (uv_stream_t *)response->handler, buf, 1, on_write);
}

void fResOkJson(fRes *res, jjson_t *json)
{
  char *body;
  enum jjson_error err = jjson_stringify(json, 2, &body);
  assert(err == JJE_OK);
  size_t body_size = strlen(body);

  size_t res_size = snprintf(NULL, 0, JSON_RESPONSE_TEMPLATE, body_size, body) + 1;
  char res_buf[res_size];
  snprintf(res_buf, res_size, JSON_RESPONSE_TEMPLATE, body_size, body);

  uv_buf_t *buf = (uv_buf_t *)malloc(sizeof(uv_buf_t));
  uv_write_t *write_req = (uv_write_t *)malloc(sizeof(uv_write_t));

  buf->base = res_buf;
  buf->len = res_size - 1;

  uv_write(write_req, (uv_stream_t *)res->handler, buf, 1, on_write);
}

int fListen(fApp *app, char *host, unsigned int port, fOnListen cb)
{
  // Init app
  app->loop = uv_default_loop();
  uv_tcp_init(app->loop, &app->socket);

  struct sockaddr_in address;
  uv_ip4_addr(host, port, &address);

  int result = uv_tcp_bind(&app->socket, (const struct sockaddr *)&address, 0);
  if (result)
  {
    fprintf(stderr, "Bind failed, %s\n", uv_strerror(result));
    return -1;
  }

  result = uv_listen((uv_stream_t *)&app->socket, 1024, on_connection);
  if (result)
  {
    fprintf(stderr, "Listen failed, %s\n", uv_strerror(result));
    return -1;
  }

  if (cb)
    cb();

  return uv_run(app->loop, UV_RUN_DEFAULT);
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

int on_method(llhttp_t *p, const char *at, size_t len)
{
  fReq *req = (fReq *)p->data;

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

  // printf("%.*s\n", (int)len, at);
  return HPE_INVALID_METHOD;
}

int on_body(llhttp_t *p, const char *at, size_t len)
{
  fReq *req = (fReq *)p->data;
  req->body = malloc(sizeof(char) * (len + 1));
  strncpy((char *)req->body, at, len);
  ((char *)req->body)[len] = '\0';
  return HPE_OK;
}

int on_url(llhttp_t *p, const char *at, size_t len)
{
  fReq *req = (fReq *)p->data;

  req->path = malloc(sizeof(char) * (len + 1));
  strncpy(req->path, at, len);
  req->path[len] = '\0';

  // printf("%.*s\n", (int)len, at);
  return HPE_OK;
}

/**
 * Match an already parsed request to a user defined route to be handled
 */
void match_request_handler(fReq *request)
{
  Route *match_route;

  HASH_FIND_STR(Routes_glob, make_route_id(request->path, request->method), match_route);

  if (!match_route || match_route->method != request->method)
  {
    // TODOO: respond with 404
    return;
  }

  if (request->method == FHTTP_POST && match_route->schema)
  {
    printf("%s\n", (char *)request->body);

    jjson_t *json = (jjson_t *)malloc(sizeof(jjson_t));
    jjson_init(json);

    enum jjson_error err = jjson_parse(json, request->body);
    if (JJE_OK != err)
      assert(!"Not implemented");

    for (int i = 0; i < match_route->schema->nfields; ++i)
    {
      const fField *field = &match_route->schema->fields[i];
      jjson_value *match_val;
      err = jjson_get(json, field->name, &match_val);
      if (JJE_OK != err || match_val->type != field->type)
        assert(!"Not implemented");
    }

    free(request->body);
    request->body = json;
  }

  fRes *response = (fRes *)malloc(sizeof(fRes));
  response->handler = request->handler;
  match_route->handler(request, response);
}

/**
 * Parse raw buffer populating the request instance with relevant data
 *
 */
void parse_request(char *raw_buffer, size_t buffer_size, fReq *request)
{
  llhttp_t parser;
  llhttp_settings_t settings;
  llhttp_settings_init(&settings);

  settings.on_url = on_url;
  settings.on_method = on_method;
  settings.on_body = on_body;

  llhttp_init(&parser, HTTP_REQUEST, &settings);
  parser.data = request;

  enum llhttp_errno _ = llhttp_execute(&parser, raw_buffer, buffer_size);

  /**
   * At this point the request has been parsed and the raw buffer is not longer needed
   */
  free(raw_buffer);

  match_request_handler(request);
}

/**
 * A callback run after reading the client socket has completed
 *
 */
void on_read_request(uv_stream_t *client, long nread, const uv_buf_t *buf)
{
  if (nread > 0)
  {
    // fprintf(stderr, "REQUEST:\n%s\n", buf->base);
    fReq *req = (fReq *)malloc(sizeof(fReq));
    req->handler = (uv_handle_t *)client;
    parse_request(buf->base, buf->len, req);
  }
  else if (nread < 0)
  {
    if (nread != UV_EOF)
    {
      fprintf(stderr, "Failed to read request, %s\n", uv_strerror(nread));
    }
    uv_close((uv_handle_t *)client, on_close_connection);
  }
}

/**
 * A callback run after closing the client socket has completed
 *
 */
void on_close_connection(uv_handle_t *client)
{
  free(client);
}
