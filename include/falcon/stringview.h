#ifndef __FALCON_STRING_VIEW__
#define __FALCON_STRING_VIEW__

#include <stdbool.h>
#include <stddef.h>

#include <falcon/errn.h>
#include <falcon/utils.h>

typedef struct
{
  const char *ptr;
  size_t len;
} fc_stringview_t;

fc_errno fc_stringview_init(const char *ptr, size_t len, fc_stringview_t *sv);
fc_errno fc_stringview_get(char **out, fc_stringview_t *sv);

fc_errno fc_string_clone(char **out, const char *src, size_t len);
bool fc_is_string_valid(const char *string);

#endif // !__FALCON_STRING_VIEW__
