#ifndef __FALCON_HTTP__
#define __FALCON_HTTP__

#include <falcon/errn.h>

#include <jack.h>
#include <uv.h>

typedef enum
{
  FHTTP_GET,
  FHTTP_POST,

  /* Must be the last one */
  __FHTTP_METHODS_COUNT__
} fhttp_method;

typedef enum
{
  FSTATUS_OK = 200,
  FSTATUS_CREATED = 201,
  FSTATUS_BAD_REQ = 400,
  FSTATUS_NOT_FOUND = 404,
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
} falcon_t;

typedef void (*fon_listen_t)();

fc_errno falcon_init(falcon_t *app);

void fget(falcon_t *app, char *path, froute_handler_t handler);
void fpost(falcon_t *app, char *path, froute_handler_t handler);
void fres_ok(fresponse_t *res);
void fres_set_status(fresponse_t *res, fhttp_status status);
void fres_json(fresponse_t *res, jjson_t *json);
int flisten(falcon_t *app, char *host, unsigned int port, fon_listen_t cb);

#endif // __FALCON_HTTP__
