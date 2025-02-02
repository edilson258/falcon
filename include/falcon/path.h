#ifndef __PATH__
#define __PATH__

#include <stddef.h>

#define PATH_MAX_LEN 5120
#define PATH_MAX_FRAGS 100
#define FRAG_LABEL_LEN 255

typedef struct
{
  int is_parameter;
  char label[FRAG_LABEL_LEN];
} frag_t;

typedef struct
{
  size_t nfrags;
  frag_t frags[PATH_MAX_FRAGS];
} fpath_t;

fpath_t *parse_path(const char *content);

#endif // !__PATH__
