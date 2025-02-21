#ifndef __FALCON__
#define __FALCON__

#include <falcon/errn.h>
#include <falcon/http.h>
#include <falcon/request.h>
#include <falcon/router.h>
#include <falcon/schema.h>
#include <falcon/stringview.h>

#include <jack.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct fc_app fc_app;

fc_app *fc_app_new();

typedef void (*fc_on_listen)();

fc_errno fc_use_router(fc_app *app, fc_router *router);

void fc_get(fc_router *router, char *path, fc_route_handler_fn handler, const fc_schema_t *schema);
void fc_post(fc_router *router, char *path, fc_route_handler_fn handler, const fc_schema_t *schema);
void fc_put(fc_router *router, char *path, fc_route_handler_fn handler, const fc_schema_t *schema);
void fc_patch(fc_router *router, char *path, fc_route_handler_fn handler, const fc_schema_t *schema);
void fc_delete(fc_router *router, char *path, fc_route_handler_fn handler, const fc_schema_t *schema);
void fc_trace(fc_router *router, char *path, fc_route_handler_fn handler, const fc_schema_t *schema);
void fc_connect(fc_router *router, char *path, fc_route_handler_fn handler, const fc_schema_t *schema);
void fc_options(fc_router *router, char *path, fc_route_handler_fn handler, const fc_schema_t *schema);
void fc_head(fc_router *router, char *path, fc_route_handler_fn handler, const fc_schema_t *schema);

void fc_res_ok(fc_response_t *res);
void fc_res_set_status(fc_response_t *res, fc_http_status status);
void fc_res_json(fc_response_t *res, jjson_t *json);
fc_errno fc_res_sendfile(fc_response_t *res, const char *path);

int fc_listen(fc_app *app, char *host, unsigned int port, fc_on_listen cb);

fc_errno fc_req_get_param(fc_request_t *req, const char *name, char **out, size_t *out_len);
fc_errno fc_req_bind_json(fc_request_t *req, jjson_t *json, const fc_schema_t *schema);
fc_errno fc_req_get_header(fc_request_t *req, const char *name, char **out, size_t *out_len);
fc_errno fc_req_get_cookie(fc_request_t *req, const char *name, char **out, size_t *out_len);

#endif // !__FALCON__
