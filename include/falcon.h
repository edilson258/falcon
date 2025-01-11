#ifndef __FALCON_HTTP__
#define __FALCON_HTTP__

#include <uv.h>

typedef enum
{
  FHTTP_GET = 1,
  FHTTP_POST = 2,
} fHttpMethod;

typedef enum
{
  FHTTP_STATUS_OK = 200,
  FHTTP_STATUS_NOT_FOUND = 404,
} fHttpStatus;

typedef struct
{
  char *path;
  fHttpMethod method;
  char *body;

  uv_handle_t *handler;
} fReq;

typedef struct
{
  uv_handle_t *handler;
} fRes;

typedef struct
{
  uv_loop_t *loop;
  uv_tcp_t socket;
} fApp;

typedef void (*falcon_route_handler)(fReq *, fRes *);
typedef void (*falcon_on_listen)();

void fGet(fApp *app, char *path, falcon_route_handler handler);
void fResOk(fRes *response);
int fListen(fApp *app, char *host, unsigned int port, falcon_on_listen cb);

#endif // __FALCON_HTTP__
