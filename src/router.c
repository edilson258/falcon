#include <falcon/errn.h>
#include <falcon/router.h>
#include <falcon/stringview.h>

#include <assert.h>
#include <ctype.h>
#include <regex.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Internal Functions */
fc_errno fc_frag_init(const char *label, fc_route_frag_t type, fc_route_frag *frag);
fc_errno fc_normalize_path_inplace(char **input);
static void frag_deinit(fc_route_frag *frag);
static char **path_split(char *path, size_t *count);
static bool route_conflict_check(fc_route_frag *existing, fc_route_frag *new_frag);

/* Router Initialization */
fc_errno fc_router_init(fc_router_t *router)
{
  fc_errno err = FC_ERR_OK;
  fc_frag_init("", FROUTE_FRAG_STATIC, &router->root);
  return err;
}

fc_errno fc_frag_init(const char *label, fc_route_frag_t type, fc_route_frag *frag)
{
  fc_errno err = FC_ERR_OK;
  frag->label = strdup(label);
  frag->type = type;
  frag->next = NULL;
  frag->children = NULL;
  return err;
}

/* Recursive frag Destruction */
static void frag_deinit(fc_route_frag *frag)
{
  if (!frag)
    return;
  free((void *)frag->label);
  frag_deinit(frag->children);
  frag_deinit(frag->next);
  free(frag);
}

fc_errno fc_normalize_path_inplace(char **input)
{
  if (!(*input) || strnlen(*input, STRING_MAX_LEN) < 1)
  {
    return fc_string_clone(input, "/", 1);
  }

  size_t read = 0, write = 0;
  bool prev_slash = false;

  while ((*input)[read])
  {
    if ((*input)[read] == '?' || (*input)[read] == '#')
      break;

    char c = (char)tolower((*input)[read++]);

    if (isspace(c))
      continue;

    if (c == '/')
    {
      if (prev_slash)
        continue;
      prev_slash = true;
    }
    else
    {
      prev_slash = false;
    }

    (*input)[write++] = c;
  }

  /* Trim trailing slash */
  if (write > 1 && (*input)[write - 1] == '/')
    write--;

  (*input)[write] = '\0';
  return FC_ERR_OK;
}

/* Path Splitting */
static char **path_split(char *path, size_t *count)
{
  assert(FC_ERR_OK == fc_normalize_path_inplace(&path));

  size_t capacity = 4;
  char **segments = malloc(sizeof(char *) * capacity);
  char *token = strtok(path, "/");
  *count = 0;

  while (token)
  {
    if (*count >= capacity)
    {
      capacity *= 2;
      char **new_segments = realloc(segments, capacity * sizeof(char *));
      if (!new_segments)
        goto error;
      segments = new_segments;
    }

    segments[*count] = strdup(token);

    if (!segments[*count])
      goto error;

    (*count)++;
    token = strtok(NULL, "/");
  }

  free(path);
  return segments;

error:
  free(path);
  for (size_t i = 0; i < *count; i++)
    free(segments[i]);
  free(segments);
  return NULL;
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

bool fc_router_add_route(fc_router_t *router, fhttp_method method, char *path, froute_handler_t handler)
{
  if (!router || method >= __FHTTP_METHODS_COUNT__ || !path || !handler)
    return false;

  size_t seg_count = 0;
  char **segments = path_split(path, &seg_count);
  if (!segments)
    return false;

  fc_route_frag *current = &router->root;
  bool wildcard_seen = false;

  for (size_t i = 0; i < seg_count; i++)
  {
    fc_route_frag_t type = FROUTE_FRAG_STATIC;
    const char *label = segments[i];

    if (wildcard_seen)
    {
      free(segments[i]);
      goto error;
    }

    if (label[0] == ':')
    {
      type = FROUTE_FRAG_PARAM;
    }
    else if (strcmp(label, "**") == 0)
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
        fprintf(stderr, "Route conflict at segment: %s\n", label);
        goto error;
      }

      if ((*child_ptr)->type == type && (type != FROUTE_FRAG_STATIC || strcmp((*child_ptr)->label, label) == 0))
      {
        current = *child_ptr;
        free(segments[i]);
        goto next_segment;
      }

      child_ptr = &(*child_ptr)->next;
    }

    fc_route_frag *new_frag = malloc(sizeof(fc_route_frag));
    fc_frag_init(label, type, new_frag);

    *child_ptr = new_frag;
    current = new_frag;
    free(segments[i]);

  next_segment:;
  }

  current->handlers[method] = handler;
  free(segments);
  return true;

error:
  for (size_t i = 0; i < seg_count; i++)
    free(segments[i]);
  free(segments);
  return false;
}

bool fc_router_match_req(fc_router_t *router, fhttp_method method, char *path, froute_handler_t *handler)
{
  if (!router || method >= __FHTTP_METHODS_COUNT__ || !path || !handler)
    return false;

  size_t seg_count = 0;
  char **segments = path_split(path, &seg_count);
  if (!segments)
    return false;

  fc_route_frag *current = &router->root;

  for (size_t i = 0; i < seg_count; i++)
  {
    fc_route_frag *child = current->children;
    bool found = false;

    while (child && !found)
    {
      switch (child->type)
      {
      case FROUTE_FRAG_STATIC:
        found = (strcmp(child->label, segments[i]) == 0);
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
      for (size_t j = 0; j < seg_count; j++)
        free(segments[j]);
      free(segments);
      return false;
    }
  }

  *handler = current->handlers[method];
  for (size_t i = 0; i < seg_count; i++)
    free(segments[i]);
  free(segments);
  return *handler != NULL;
}
