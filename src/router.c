#include <assert.h>
#include <ctype.h>
#include <regex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <time.h>

#include <falcon/errn.h>
#include <falcon/router.h>
#include <falcon/stringview.h>

#define FC__PATH_MAX_FRAGS 100

/* Internal Functions */
static fc_errno fc__frag_init(const char *label, fc_route_frag_t type, fc_route_frag *frag);
static fc_errno fc__normalize_path_inplace(char **input);
static fc_errno fc__split_path(char *path, size_t *count, char *raw_frags[FC__PATH_MAX_FRAGS]);
static bool route_conflict_check(fc_route_frag *existing, fc_route_frag *new_frag);

/* Router Initialization */
fc_errno fc_router_init(fc_router_t *router)
{
  fc__frag_init("", FROUTE_FRAG_STATIC, &router->root);
  return FC_ERR_OK;
}

fc_errno fc__frag_init(const char *label, fc_route_frag_t type, fc_route_frag *frag)
{
  frag->label = strdup(label);
  frag->type = type;
  frag->next = NULL;
  frag->children = NULL;
  return FC_ERR_OK;
}

fc_errno fc__normalize_path_inplace(char **input)
{
  if (!(*input) || strnlen(*input, STRING_MAX_LEN) < 1)
  {
    return fc_string_clone(input, "/", 1);
  }

  bool prev_slash = false;
  size_t read = 0, write = 0;

  while ((*input)[read])
  {
    char c = (char)tolower((*input)[read++]);
    if (isspace(c) || (c == '/' && prev_slash))
      continue;
    prev_slash = c == '/' ? true : false;
    (*input)[write++] = c;
  }

  /* Trim trailing slash */
  if (write > 1 && (*input)[write - 1] == '/')
    write--;
  (*input)[write] = '\0';

  return FC_ERR_OK;
}

fc_errno fc__split_path(char *path, size_t *count, char *raw_frags[FC__PATH_MAX_FRAGS])
{
  assert(FC_ERR_OK == fc__normalize_path_inplace(&path));

  *count = 0;
  char *token = strtok(path, "/");

  while (token)
  {
    if (*count >= FC__PATH_MAX_FRAGS)
    {
      return FC_ERR_STRING_TOO_LONG;
    }
    raw_frags[*count] = token;
    (*count)++;
    token = strtok(NULL, "/");
  }

  return FC_ERR_OK;
}

/* Route Conflict Detection */
static bool route_conflict_check(fc_route_frag *existing, fc_route_frag *new_frag)
{
  if (existing->type == FROUTE_FRAG_STATIC && new_frag->type == FROUTE_FRAG_STATIC)
    return strcmp(existing->label, new_frag->label) == 0;

  if (existing->type != FROUTE_FRAG_STATIC && new_frag->type != FROUTE_FRAG_STATIC)
    return true;

  return false;
}

bool fc_router_add_route(fc_router_t *router, fc_http_method method, char *path, fc_route_handler_t handler)
{
  size_t raw_frags_count = 0;
  char *raw_frags[FC__PATH_MAX_FRAGS];
  assert(FC_ERR_OK == fc__split_path(path, &raw_frags_count, (char **)raw_frags));

  bool wildcard_seen = false;
  fc_route_frag *current = &router->root;

  for (size_t i = 0; i < raw_frags_count; i++)
  {
    fc_route_frag_t type = FROUTE_FRAG_STATIC;
    const char *raw_frag = raw_frags[i];

    if (wildcard_seen)
    {
      return false;
    }

    if (raw_frag[0] == ':')
    {
      type = FROUTE_FRAG_PARAM;
    }
    else if (strncmp(raw_frag, "**", MIN(2, strnlen(raw_frag, STRING_MAX_LEN))) == 0)
    {
      type = FROUTE_FRAG_WILDCARD;
      wildcard_seen = true;
    }

    /* Conflict checking */
    fc_route_frag **child_ptr = &current->children;
    while (*child_ptr)
    {
      if (route_conflict_check(*child_ptr, current))
      {
        fprintf(stderr, "Route conflict at segment: %s\n", raw_frag);
        return false;
      }

      if ((*child_ptr)->type == type && (type != FROUTE_FRAG_STATIC || strcmp((*child_ptr)->label, raw_frag) == 0))
      {
        current = *child_ptr;
        goto next_segment;
      }

      child_ptr = &(*child_ptr)->next;
    }

    fc_route_frag *new_frag = malloc(sizeof(fc_route_frag));
    fc__frag_init(raw_frag, type, new_frag);

    *child_ptr = new_frag;
    current = new_frag;

  next_segment:;
  }

  current->handlers[method] = handler;
  return true;
}

bool fc_router_match_req(fc_router_t *router, fc_http_method method, char *path, fc_route_handler_t *handler)
{
  if (!router || method >= __FC_HTTP_METHODS_COUNT__ || !path || !handler)
    return false;

  size_t raw_frags_count = 0;
  char *raw_frags[FC__PATH_MAX_FRAGS];
  assert(FC_ERR_OK == fc__split_path(path, &raw_frags_count, (char **)raw_frags));

  fc_route_frag *current = &router->root;

  for (size_t i = 0; i < raw_frags_count; i++)
  {
    fc_route_frag *child = current->children;
    bool found = false;

    while (child && !found)
    {
      switch (child->type)
      {
      case FROUTE_FRAG_STATIC:
        found = (strcmp(child->label, raw_frags[i]) == 0);
        break;

      case FROUTE_FRAG_PARAM:
      case FROUTE_FRAG_WILDCARD:
        found = true;
        break;
      }

      if (found)
      {
        current = child;
        break;
      }

      child = child->next;
    }

    if (!found)
    {
      return false;
    }
  }

  *handler = current->handlers[method];
  return *handler != NULL;
}
