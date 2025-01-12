#ifndef __FALCON_HTTP__
#define __FALCON_HTTP__

#include <jack.h>
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
  void *body;

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

typedef struct
{
  char *name;
  jjson_type type;
} fField;

typedef struct
{
  unsigned nfields;
  fField fields[];
} fSchema;

typedef void (*fOnListen)();
typedef void (*fRouteHandler)(fReq *, fRes *);

void fGet(fApp *app, char *path, fRouteHandler handler);
void fPost(fApp *app, char *path, fRouteHandler handler, fSchema *schema);
void fResOk(fRes *res);
void fResOkJson(fRes *res, jjson_t *json);
int fListen(fApp *app, char *host, unsigned int port, fOnListen cb);

#endif // __FALCON_HTTP__
