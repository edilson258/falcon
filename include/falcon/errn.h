#ifndef __FALCON_ERRN__
#define __FALCON_ERRN__

typedef enum
{
  FERR_OK = 0,
  FERR_TOO_MANY_FRAGS = 1,
  FERR_INVALID_TOKEN = 2,
  FERR_STRING_TOO_LONG = 3,
} ferrno;

#endif // !__FALCON_ERRN__
