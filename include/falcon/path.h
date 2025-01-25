#ifndef __PATH__
#define __PATH__

#include <stddef.h>

#define PATH_MAX_LEN 5120

typedef struct
{
  char *name;
  int is_dyn;
} frag_t;

typedef struct
{
  size_t nfrags;
  frag_t frags[];
} fpath_t;

int parse_path(char *raw, fpath_t *path);

#endif // !__PATH__
