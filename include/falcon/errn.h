#ifndef __FALCON_ERRN__
#define __FALCON_ERRN__

typedef enum
{
  FC_ERR_OK = 0,
  FC_ERR_TOO_MANY_FRAGS = 1,
  FC_ERR_INVALID_TOKEN = 2,
  FC_ERR_STRING_TOO_LONG = 3,
  FC_ERR_STRING_TOO_SHORT = 4,
  FC_ERR_INVALID_STRING = 5,
} fc_errno;

#endif // !__FALCON_ERRN__
