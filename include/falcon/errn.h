#ifndef __FALCON_ERRN__
#define __FALCON_ERRN__

typedef enum
{
  FC_ERR_OK = 0,
  FC_ERR_LIMIT_EXCEEDED = 1,
  FC_ERR_INVALID_TOKEN = 2,
  FC_ERR_STRING_TOO_LONG = 3,
  FC_ERR_STRING_TOO_SHORT = 4,
  FC_ERR_INVALID_STRING = 5,
  FC_ERR_ROUTE_CONFLIT = 6,
  FC_ERR_ENTRY_NOT_FOUND = 7,
  FC_ERR_INVALID_ROUTE = 8,
  FC_ERR_INVALID_JSON = 9,
} fc_errno;

#endif // !__FALCON_ERRN__
