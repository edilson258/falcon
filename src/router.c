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

#define PATH_MAX_FRAGS 100

/* Internal Functions */
static fc_errno fc__frag_init(const char *label, fc__route_frag_t type, fc__route_frag *frag);
static fc_errno fc__normalize_path_inplace(char **input);
static fc_errno fc__split_path(char *path, size_t *count, char *raw_frags[PATH_MAX_FRAGS]);
static bool fc__check_route_conflict(fc__route_frag *existing, fc__route_frag *new_frag);

/* Router Initialization */
fc_errno fc__router_init(fc_router_t *router)
{
  fc__frag_init("", FC__ROUTE_FRAG_STATIC, &router->root);
  return FC_ERR_OK;
}

fc_errno fc__frag_init(const char *label, fc__route_frag_t type, fc__route_frag *frag)
{
  frag->label = strdup(label);
  frag->type = type;
  frag->next = NULL;
  frag->children = NULL;
  return FC_ERR_OK;
}

fc_errno fc__normalize_path_inplace(char **input)
{
  if (!(*input) || !(**input))
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

fc_errno fc__split_path(char *path, size_t *count, char *raw_frags[PATH_MAX_FRAGS])
{
  fc_errno err = fc__normalize_path_inplace(&path);
  if (FC_ERR_OK != err)
    return err;

  *count = 0;
  char *token = strtok(path, "/");

  while (token)
  {
    if (*count >= PATH_MAX_FRAGS)
      return FC_ERR_STRING_TOO_LONG;
    raw_frags[*count] = token;
    (*count)++;
    token = strtok(NULL, "/");
  }

  return FC_ERR_OK;
}

/* Route Conflict Detection */
static bool fc__check_route_conflict(fc__route_frag *existing, fc__route_frag *new_frag)
{
  if (existing->type == FC__ROUTE_FRAG_STATIC && new_frag->type == FC__ROUTE_FRAG_STATIC)
    return strncmp(existing->label, new_frag->label, STRING_MAX_LEN) == 0;

  if (existing->type != FC__ROUTE_FRAG_STATIC && new_frag->type != FC__ROUTE_FRAG_STATIC)
    return true;

  return false;
}

fc_errno fc__router_add_route(fc_router_t *router, fc_http_method method, char *path, fc_route_handler_t handler)
{
  size_t raw_frags_count = 0;
  char *raw_frags[PATH_MAX_FRAGS];
  fc_errno err = fc__split_path(path, &raw_frags_count, (char **)raw_frags);
  if (FC_ERR_OK != err)
    return err;

  bool wildcard_seen = false;
  fc__route_frag *current = &router->root;

  for (size_t i = 0; i < raw_frags_count; i++)
  {
    fc__route_frag_t type = FC__ROUTE_FRAG_STATIC;
    const char *raw_frag = raw_frags[i];

    if (wildcard_seen)
      return FC_ERR_INVALID_ROUTE;

    if (raw_frag[0] == ':')
      type = FC__ROUTE_FRAG_PARAM;
    else if (strncmp(raw_frag, "**", MIN(2, strnlen(raw_frag, STRING_MAX_LEN))) == 0)
    {
      type = FC__ROUTE_FRAG_WILDCARD;
      wildcard_seen = true;
    }

    /* Conflict checking */
    fc__route_frag **child_ptr = &current->children;
    while (*child_ptr)
    {
      if (fc__check_route_conflict(*child_ptr, current))
        return FC_ERR_ROUTE_CONFLIT;

      if ((*child_ptr)->type == type && (type != FC__ROUTE_FRAG_STATIC || strncmp((*child_ptr)->label, raw_frag, STRING_MAX_LEN) == 0))
      {
        current = *child_ptr;
        goto next_fragment;
      }

      child_ptr = &(*child_ptr)->next;
    }

    fc__route_frag *new_frag = malloc(sizeof(fc__route_frag));
    fc__frag_init(raw_frag, type, new_frag);

    *child_ptr = new_frag;
    current = new_frag;

  next_fragment:;
  }

  current->handlers[method] = handler;
  return FC_ERR_OK;
}

fc_errno fc__router_match_req(fc_router_t *router, fc_http_method method, char *path, fc_route_handler_t *handler)
{
  size_t raw_frags_count = 0;
  char *raw_frags[PATH_MAX_FRAGS];
  fc_errno err = fc__split_path(path, &raw_frags_count, (char **)raw_frags);
  if (FC_ERR_OK != err)
    return err;

  fc__route_frag *current = &router->root;

  for (size_t i = 0; i < raw_frags_count; i++)
  {
    fc__route_frag *child = current->children;
    bool found = false;

    while (child && !found)
    {
      switch (child->type)
      {
      case FC__ROUTE_FRAG_STATIC:
        found = (strncmp(child->label, raw_frags[i], STRING_MAX_LEN) == 0);
        break;
      case FC__ROUTE_FRAG_PARAM:
      case FC__ROUTE_FRAG_WILDCARD:
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
      return FC_ERR_ROUTE_NOT_FOUND;
  }

  *handler = current->handlers[method];
  return *handler == NULL ? FC_ERR_ROUTE_NOT_FOUND : FC_ERR_OK;
}
