#ifndef __FALCON_SCHEMA__
#define __FALCON_SCHEMA__

#include <stddef.h>

typedef enum
{
  FC_ANY_T = 0,
  FC_CHAR_T = 1,
  FC_STRING_T = 2,
  FC_INTEGER_T = 3,
  FC_FLOAT_T = 4,
  FC_JSON_T = 5,
  FC_ARRAY_T = 6,
} fc_data_t;

typedef struct
{
  fc_data_t type;
  const char *name;
} fc_field_t;

#define FC__SCHEMA_FIELD_COUNT 100

typedef struct
{
  size_t nfields;
  fc_field_t fields[FC__SCHEMA_FIELD_COUNT];
} fc_schema_t;

#endif // !__FALCON_SCHEMA__
