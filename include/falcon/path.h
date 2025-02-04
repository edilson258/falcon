#ifndef __FALCON_PATH__
#define __FALCON_PATH__

#include <falcon.h>
#include <stddef.h>

#define PATH_MAX_LEN (32 * 1024)
#define FRAG_LABEL_LEN (5 * 1024)
#define PATH_MAX_FRAGS 1000

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

ferrno parse_path(const char *content, fpath_t *path);

#endif // !__FALCON_PATH__
