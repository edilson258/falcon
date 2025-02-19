#ifndef __FALCON_RESPONSE__
#define __FALCON_RESPONSE__

#include <falcon/http.h>

typedef struct
{
  fc_http_status status;
  struct uv_handle_t *handler;
} fc_response_t;

#endif // !__FALCON_RESPONSE__
