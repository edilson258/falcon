#ifndef __FALCON_STRING_VIEW__
#define __FALCON_STRING_VIEW__

#include <stdbool.h>
#include <stddef.h>

#include <falcon/errn.h>
#include <falcon/utils.h>

/* Stringview key-value array length */
#define FC__SV_KV_ARRAY_LEN 100

typedef struct
{
  const char *ptr;
  size_t len;
} fc_stringview_t;

/* stringview key-value */
typedef struct
{
  fc_stringview_t key;
  fc_stringview_t value;
} fc__sv_kv;

/*  key-value array of stringviews */
typedef struct
{
  size_t count;
  fc__sv_kv items[FC__SV_KV_ARRAY_LEN];
} fc__sv_kv_array;

fc_errno fc_stringview_init(const char *ptr, size_t len, fc_stringview_t *sv);
fc_errno fc_stringview_get(char **out, fc_stringview_t *sv);

fc_errno fc_string_clone(char **out, const char *src, size_t len);
bool fc_is_string_valid(const char *string);

fc_errno fc__sv_kv_array_get(fc__sv_kv_array *arr, const char *name, char **out, size_t *out_len);

#endif // !__FALCON_STRING_VIEW__
