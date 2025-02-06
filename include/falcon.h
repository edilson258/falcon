#ifndef __FALCON_HTTP__
#define __FALCON_HTTP__

#include <falcon/errn.h>
#include <falcon/stringview.h>

#include <jack.h>
#include <uv.h>

typedef enum
{
  FC_HTTP_GET,
  FC_HTTP_POST,

  /* Must be the last one */
  __FC_HTTP_METHODS_COUNT__
} fc_http_method;

typedef enum
{
  FC_STATUS_OK = 200,
  FC_STATUS_CREATED = 201,
  FC_STATUS_BAD_REQ = 400,
  FC_STATUS_NOT_FOUND = 404,
} fc_http_status;

typedef struct
{
  fc_stringview_t buf;
  fc_stringview_t path;
  fc_stringview_t body_buf;
  fc_http_method method;
  uv_handle_t *handler;
} fc_request_t;

typedef struct
{
  uv_handle_t *handler;
  fc_http_status status;
} fc_response_t;

typedef void (*fc_route_handler_t)(fc_request_t *, fc_response_t *);

typedef struct
{
} fc_t;

typedef void (*fon_listen_t)();

fc_errno fc_init(fc_t *app);

void fc_get(fc_t *app, char *path, fc_route_handler_t handler);
void fc_post(fc_t *app, char *path, fc_route_handler_t handler);
void fc_res_ok(fc_response_t *res);
void fc_res_set_status(fc_response_t *res, fc_http_status status);
void fc_res_json(fc_response_t *res, jjson_t *json);
int fc_listen(fc_t *app, char *host, unsigned int port, fon_listen_t cb);

#endif // __FALCON_HTTP__
