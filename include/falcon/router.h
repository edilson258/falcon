#ifndef __FALCON_ROUTER__
#define __FALCON_ROUTER__

#include <stdbool.h>

#include <falcon/request.h>
#include <falcon/response.h>
#include <falcon/schema.h>

#define PATH_MAX_FRAGS 100

typedef void (*fc_route_handler_fn)(fc_request_t *, fc_response_t *);

typedef enum
{
  FC__ROUTE_FRAG_STATIC = 1,
  FC__ROUTE_FRAG_PARAM = 2,
  FC__ROUTE_FRAG_WILDCARD = 3,
} fc__route_frag_t;

typedef struct
{
  const fc_schema_t *schema;
  fc_route_handler_fn handle;
} fc__route_handler;

typedef struct fc__route_frag
{
  const char *label;
  fc__route_frag_t type;
  struct fc__route_frag *next;
  struct fc__route_frag *children;
  fc__route_handler *handlers[FC__HTTP_METHODS_COUNT];
} fc__route_frag;

typedef struct fc_router fc_router;

fc_router *fc_router_new();

fc_errno fc__router_add_route(fc_router *router, fc_http_method method, char *path, fc_route_handler_fn handler, const fc_schema_t *schema);
fc_errno fc__router_match_req(fc_router *router, fc_request_t *req, char *path, fc__route_handler **handler);

/* Internal Functions */
fc_errno fc__frag_init(const char *label, fc__route_frag_t type, fc__route_frag *frag);
fc_errno fc__normalize_path_inplace(char **input);
fc_errno fc__split_path(char *path, size_t *count, char *raw_frags[PATH_MAX_FRAGS]);
bool fc__check_route_conflict(fc__route_frag *existing, fc__route_frag *new_frag);

#endif // !__FALCON_ROUTER__
