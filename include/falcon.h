#ifndef __FALCON__
#define __FALCON__

#include <falcon/errn.h>
#include <falcon/http.h>
#include <falcon/stringview.h>

#include <jack.h>
#include <stddef.h>
#include <uv.h>

#define FC__REQ_PARAM_MAX_LEN 100

typedef struct
{
  fc_stringview_t name;
  fc_stringview_t value;
} fc__req_param_t;

typedef struct fc__req_params
{
  size_t nparams;
  fc__req_param_t params[FC__REQ_PARAM_MAX_LEN];
} fc__req_params;

typedef struct
{
  fc_stringview_t buf;
  fc_stringview_t path;
  fc_stringview_t body_buf;
  void *body;
  fc_http_method method;
  uv_handle_t *handler;
  fc__req_params params;
} fc_request_t;

typedef struct
{
  uv_handle_t *handler;
  fc_http_status status;
} fc_response_t;

typedef enum
{
  FC_ANY_T = 0,
  FC_CHAR_T = 1,
  FC_STRING_T = 2,
  FC_INTEGER_T = 3,
  FC_FLOAT_T = 4,
  FC_JSON_T = 5,
  FC_ARRAY_T = 6,
} fc_data_t;

typedef struct
{
  fc_data_t type;
  const char *name;
} fc_field_t;

#define FC__SCHEMA_FIELD_COUNT 100

typedef struct
{
  size_t nfields;
  fc_field_t fields[FC__SCHEMA_FIELD_COUNT];
} fc_schema_t;

typedef void (*fc_route_handler_fn)(fc_request_t *, fc_response_t *);

typedef struct
{
} fc_t;

typedef void (*fc_on_listen)();

fc_errno fc_init(fc_t *app);

void fc_get(fc_t *app, char *path, fc_route_handler_fn handler);
void fc_post(fc_t *app, char *path, fc_route_handler_fn handler, const fc_schema_t *schema);
void fc_put(fc_t *app, char *path, fc_route_handler_fn handler, const fc_schema_t *schema);
void fc_patch(fc_t *app, char *path, fc_route_handler_fn handler, const fc_schema_t *schema);
void fc_delete(fc_t *app, char *path, fc_route_handler_fn handler, const fc_schema_t *schema);

void fc_res_ok(fc_response_t *res);
void fc_res_set_status(fc_response_t *res, fc_http_status status);
void fc_res_json(fc_response_t *res, jjson_t *json);
int fc_listen(fc_t *app, char *host, unsigned int port, fc_on_listen cb);

fc_errno fc_req_get_param(fc_request_t *req, const char *name, char **out);
fc_errno fc_req_get_param_as_int(fc_request_t *req, const char *name, int *out);
fc_errno fc_req_bind_json(fc_request_t *req, jjson_t *json, const fc_schema_t *schema);

#endif // !__FALCON__
