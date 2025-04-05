#ifndef __FALCON_REQUEST__
#define __FALCON_REQUEST__

#include <jack.h>

#include <falcon/errn.h>
#include <falcon/http.h>
#include <falcon/stringview.h>

typedef struct
{
  /* If true, cookies are already parsed */
  bool parsed;
  fc__sv_kv_array cookies;
} fc__req_cookies;

typedef struct
{
  fc_http_method method;
  fc_stringview_t raw;
  fc_stringview_t path;
  fc_stringview_t raw_body;
  jjson_t *body;
  fc__sv_kv_array params;
  fc__sv_kv_array headers;
  fc__req_cookies cookies;
  struct uv_handle_t *handler;
} fc_request_t;

fc_errno fc__req_parse_cookies(fc_request_t *req);

#endif // !__FALCON_REQUEST__
