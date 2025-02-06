#ifndef __FALCON_ROUTER__
#define __FALCON_ROUTER__

#include <falcon.h>
#include <stdbool.h>

typedef enum
{
  FROUTE_FRAG_STATIC = 1,
  FROUTE_FRAG_PARAM = 2,
  FROUTE_FRAG_WILDCARD = 3,
} fc_route_frag_t;

typedef struct fc_route_frag
{
  const char *label;
  fc_route_frag_t type;
  struct fc_route_frag *next;
  struct fc_route_frag *children;
  froute_handler_t handlers[__FHTTP_METHODS_COUNT__];
} fc_route_frag;

typedef struct
{
  fc_route_frag root;
} fc_router_t;

fc_errno fc_router_init(fc_router_t *router);

bool fc_router_add_route(fc_router_t *router, fhttp_method method, char *path, froute_handler_t handler);
bool fc_router_match_req(fc_router_t *router, fhttp_method method, char *path, froute_handler_t *handler);

#endif // !__FALCON_ROUTER__
