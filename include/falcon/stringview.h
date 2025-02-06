#ifndef __FALCON_STRING_VIEW__
#define __FALCON_STRING_VIEW__

#include <falcon/errn.h>

#include <stdbool.h>
#include <stddef.h>

#define STRING_MAX_LEN (32 * 1024)

typedef struct
{
  char *ptr;
  size_t len;
} fc_stringview;

fc_errno fc_stringview_new(char *ptr, size_t len, fc_stringview *sv);
fc_errno fc_stringview_get(fc_stringview *sv, char **out);

fc_errno fc_string_clone(char **out, const char *src, size_t len);
bool fc_is_string_valid(const char *string);

#endif // !__FALCON_STRING_VIEW__
