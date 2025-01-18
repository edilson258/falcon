#ifndef __FALCON_HTTP__
#define __FALCON_HTTP__

#include <jack.h>
#include <uthash.h>
#include <uv.h>

typedef enum
{
  FHTTP_GET = 1,
  FHTTP_POST = 2,
} fhttp_method;

typedef enum
{
  FHTTP_STATUS_OK = 200,
  FHTTP_STATUS_CREATED = 201,
  FHTTP_STATUS_BAD_REQ = 400,
  FHTTP_STATUS_NOT_FOUND = 404,
} fhttp_status;

typedef struct
{
  char *path;
  void *body;
  fhttp_method method;
  uv_handle_t *handler;
} frequest_t;

typedef struct
{
  uv_handle_t *handler;
  struct fApp *app;
  fhttp_status status;
} fresponse_t;

typedef void (*froute_handler_t)(frequest_t *, fresponse_t *);

typedef struct
{
  char *name;
  jjson_type type;
} ffield_t;

typedef struct
{
  unsigned nfields;
  ffield_t fields[];
} fschema_t;

typedef struct fRoute
{
  char *id;
  char *path;
  fschema_t *schema;
  fhttp_method method;
  froute_handler_t handler;

  UT_hash_handle hh; // make struct hashable
} fRoute;

typedef struct
{
} falcon_t;

typedef void (*fon_listen_t)();

void fget(falcon_t *app, char *path, froute_handler_t handler);
void fpost(falcon_t *app, char *path, froute_handler_t handler, fschema_t *schema);
void fres_ok(fresponse_t *res);
void fres_set_status(fresponse_t *res, fhttp_status status);
void fres_json(fresponse_t *res, jjson_t *json);
int flisten(falcon_t *app, char *host, unsigned int port, fon_listen_t cb);

#endif // __FALCON_HTTP__
