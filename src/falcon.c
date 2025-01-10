#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include <llhttp.h>
#include <uthash.h>

#include "falcon.h"

static char CONTENT_TYPE_PLAIN[] = "text/plain";
static char OK_RESPONSE[] = "HTTP/1.1 200 OK\r\n"
                            "Server: Falcon\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: 2\r\n"
                            "\r\n"
                            "Ok";

typedef struct
{
  char *path;
  falcon_http_method method;
  falcon_route_handler handler;

  UT_hash_handle hh;
} Route;

Route *Routes_glob = NULL;

void on_connection(uv_stream_t *server, int status);
void on_alloc_req_buf(uv_handle_t *client, size_t size, uv_buf_t *buf);
void on_read_request(uv_stream_t *client, long nread, const uv_buf_t *buf);
void on_close_connection(uv_handle_t *client);

// llhttp parser
void parse_request(char *raw_buffer, size_t buffer_size, falcon_request *request);
int on_url(llhttp_t *p, const char *at, size_t len);
int on_method(llhttp_t *p, const char *at, size_t len);

void falcon_get(falcon *app, char *path, falcon_route_handler handler)
{
  Route *r = (Route *)malloc(sizeof(Route));
  r->path = path;
  r->method = FALCON_HTTP_GET;
  r->handler = handler;
  HASH_ADD_STR(Routes_glob, path, r);
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

void falcon_ok(falcon_response *response)
{
  uv_buf_t *buf = (uv_buf_t *)malloc(sizeof(uv_buf_t));
  uv_write_t *write_req = (uv_write_t *)malloc(sizeof(uv_write_t));

  buf->base = OK_RESPONSE;
  buf->len = strlen(OK_RESPONSE);

  uv_write(write_req, (uv_stream_t *)response->handler, buf, 1, on_write);
}

void init_app(falcon *app)
{
  app->loop = uv_default_loop();
  uv_tcp_init(app->loop, &app->socket);
}

int falcon_listen(falcon *app, char *host, unsigned int port, falcon_on_listen cb)
{
  init_app(app);

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
  falcon_request *req = (falcon_request *)p->data;

  if (0 == strncmp("GET", at, len))
  {
    req->method = FALCON_HTTP_GET;
    return HPE_OK;
  }

  // printf("%.*s\n", (int)len, at);
  return HPE_INVALID_METHOD;
}

int on_url(llhttp_t *p, const char *at, size_t len)
{
  falcon_request *req = (falcon_request *)p->data;

  req->path = malloc(sizeof(char) * (len + 1));
  strncpy(req->path, at, len);
  req->path[len] = '\0';

  // printf("%.*s\n", (int)len, at);
  return HPE_OK;
}

/**
 * Match an already parsed request to a user defined route to be handled
 */
void match_request_handler(falcon_request *request)
{
  Route *matching_route;

  HASH_FIND_STR(Routes_glob, request->path, matching_route);

  if (!matching_route || matching_route->method != request->method)
  {
    // TODOO: respond with 404
    return;
  }

  falcon_response *response = (falcon_response *)malloc(sizeof(falcon_response));
  response->handler = request->handler;
  matching_route->handler(request, response);

  // request and response instances are no longer used
  free(request->path);
  free(request);
  free(response);
}

/**
 * Parse raw buffer populating the request instance with relevant data
 *
 */
void parse_request(char *raw_buffer, size_t buffer_size, falcon_request *request)
{
  llhttp_t parser;
  llhttp_settings_t settings;
  llhttp_settings_init(&settings);

  settings.on_url = on_url;
  settings.on_method = on_method;

  llhttp_init(&parser, HTTP_REQUEST, &settings);
  parser.data = request;

  enum llhttp_errno _ = llhttp_execute(&parser, raw_buffer, buffer_size);

  // if (err == HPE_OK)
  // {
  //   fprintf(stdout, "Successfully parsed!\n");
  // }
  // else
  // {
  //   fprintf(stderr, "Parse error: %s %s\n", llhttp_errno_name(err), parser.reason);
  // }

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
    falcon_request *req = (falcon_request *)malloc(sizeof(falcon_request));
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
void on_close_connection(uv_handle_t *client) { free(client); }
