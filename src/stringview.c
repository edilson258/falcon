#include <falcon/stringview.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

fc_errno fc_stringview_new(char *ptr, size_t len, fc_stringview *sv)
{
  fc_errno err = FC_ERR_OK;
  sv->ptr = ptr;
  sv->len = len;
  return err;
}

fc_errno fc_stringview_get(fc_stringview *sv, char **out)
{
  fc_errno err = FC_ERR_OK;
  (*out) = malloc(sizeof(char) * (sv->len + 1));
  strncpy(*out, sv->ptr, sv->len);
  (*out)[sv->len] = '\0';
  return err;
}

fc_errno fc_string_clone(char **out, const char *src, size_t given_len)
{
  if (!src)
  {
    return FC_ERR_INVALID_STRING;
  }

  size_t len = MIN(strnlen(src, MAX_STRING_LEN), given_len);

  (*out) = malloc(sizeof(char) * (len) + 1);
  strncpy(*out, src, len);
  (*out)[len] = '\0';

  return FC_ERR_OK;
}

bool fc_is_string_valid(const char *string)
{
  return string ? true : false;
}

bool fc_is_string_empty(const char *string)
{
  if (fc_is_string_valid(string))
  {
    return *string ? true : false;
  }
  return true;
}
