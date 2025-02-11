#include <falcon/stringview.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

fc_errno fc_stringview_init(const char *ptr, size_t len, fc_stringview_t *sv)
{
  if (!fc_is_string_valid(ptr))
    return FC_ERR_INVALID_STRING;

  size_t _len = MIN(strnlen(ptr, STRING_MAX_LEN), len);
  if (len < 1)
    return FC_ERR_STRING_TOO_SHORT;

  sv->ptr = ptr;
  sv->len = _len;
  return FC_ERR_OK;
}

fc_errno fc_stringview_get(char **out, fc_stringview_t *sv)
{
  if (!fc_is_string_valid(sv->ptr))
    return FC_ERR_INVALID_STRING;

  (*out) = strndup(sv->ptr, sv->len);
  return FC_ERR_OK;
}

fc_errno fc_string_clone(char **out, const char *src, size_t given_len)
{
  if (!fc_is_string_valid(src))
    return FC_ERR_INVALID_STRING;
  (*out) = strndup(src, MIN(strnlen(src, STRING_MAX_LEN), given_len));
  return FC_ERR_OK;
}

bool fc_is_string_valid(const char *string)
{
  return string ? true : false;
}
