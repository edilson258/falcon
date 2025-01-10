#ifndef __FALCON_HTTP__
#define __FALCON_HTTP__

#include <uv.h>

typedef enum
{
  FALCON_HTTP_GET = 1,
} falcon_http_method;

typedef enum
{
  FALCON_HTTP_STATUS_OK = 200,
  FALCON_HTTP_STATUS_NOT_FOUND = 404,
} falcon_http_status;

typedef struct
{
  char *path;
  falcon_http_method method;
  uv_handle_t *handler;
} falcon_request;

typedef struct
{
  uv_handle_t *handler;
} falcon_response;

typedef struct
{
  uv_loop_t *loop;
  uv_tcp_t socket;
} falcon;

typedef void (*falcon_route_handler)(falcon_request *, falcon_response *);
typedef void (*falcon_on_listen)();

void falcon_get(falcon *app, char *path, falcon_route_handler handler);
void falcon_ok(falcon_response *response);
int falcon_listen(falcon *app, char *host, unsigned int port, falcon_on_listen cb);

#endif // __FALCON_HTTP__
