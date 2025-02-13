#include <falcon/http.h>

char *fc__http_status_str(fc_http_status status)
{
  switch (status)
  {
  /* 1xx: Informational */
  case FC_STATUS_CONTINUE:
    return "Continue";
  case FC_STATUS_SWITCHING_PROTOCOLS:
    return "Switching Protocols";
  case FC_STATUS_PROCESSING:
    return "Processing";
  case FC_STATUS_EARLY_HINTS:
    return "Early Hints";

  /* 2xx: Success */
  case FC_STATUS_OK:
    return "OK";
  case FC_STATUS_CREATED:
    return "Created";
  case FC_STATUS_ACCEPTED:
    return "Accepted";
  case FC_STATUS_NON_AUTHORITATIVE_INFORMATION:
    return "Non-Authoritative Information";
  case FC_STATUS_NO_CONTENT:
    return "No Content";
  case FC_STATUS_RESET_CONTENT:
    return "Reset Content";
  case FC_STATUS_PARTIAL_CONTENT:
    return "Partial Content";
  case FC_STATUS_MULTI_STATUS:
    return "Multi-Status";
  case FC_STATUS_ALREADY_REPORTED:
    return "Already Reported";
  case FC_STATUS_IM_USED:
    return "IM Used";

  /* 3xx: Redirection */
  case FC_STATUS_MULTIPLE_CHOICES:
    return "Multiple Choices";
  case FC_STATUS_MOVED_PERMANENTLY:
    return "Moved Permanently";
  case FC_STATUS_FOUND:
    return "Found";
  case FC_STATUS_SWITCH_PROXY:
    return "Switching proxy";
  case FC_STATUS_SEE_OTHER:
    return "See Other";
  case FC_STATUS_NOT_MODIFIED:
    return "Not Modified";
  case FC_STATUS_USE_PROXY:
    return "Use Proxy";
  case FC_STATUS_TEMPORARY_REDIRECT:
    return "Temporary Redirect";
  case FC_STATUS_PERMANENT_REDIRECT:
    return "Permanent Redirect";

  /* 4xx: Client Errors */
  case FC_STATUS_BAD_REQUEST:
    return "Bad Request";
  case FC_STATUS_UNAUTHORIZED:
    return "Unauthorized";
  case FC_STATUS_PAYMENT_REQUIRED:
    return "Payment Required";
  case FC_STATUS_FORBIDDEN:
    return "Forbidden";
  case FC_STATUS_NOT_FOUND:
    return "Not Found";
  case FC_STATUS_METHOD_NOT_ALLOWED:
    return "Method Not Allowed";
  case FC_STATUS_NOT_ACCEPTABLE:
    return "Not Acceptable";
  case FC_STATUS_PROXY_AUTH_REQUIRED:
    return "Proxy Authentication Required";
  case FC_STATUS_REQUEST_TIMEOUT:
    return "Request Timeout";
  case FC_STATUS_CONFLICT:
    return "Conflict";
  case FC_STATUS_GONE:
    return "Gone";
  case FC_STATUS_LENGTH_REQUIRED:
    return "Length Required";
  case FC_STATUS_PRECONDITION_FAILED:
    return "Precondition Failed";
  case FC_STATUS_PAYLOAD_TOO_LARGE:
    return "Payload Too Large";
  case FC_STATUS_URI_TOO_LONG:
    return "URI Too Long";
  case FC_STATUS_UNSUPPORTED_MEDIA_TYPE:
    return "Unsupported Media Type";
  case FC_STATUS_RANGE_NOT_SATISFIABLE:
    return "Range Not Satisfiable";
  case FC_STATUS_EXPECTATION_FAILED:
    return "Expectation Failed";
  case FC_STATUS_IM_A_TEAPOT:
    return "I'm a teapot";
  case FC_STATUS_MISDIRECTED_REQUEST:
    return "Misdirected Request";
  case FC_STATUS_UNPROCESSABLE_ENTITY:
    return "Unprocessable Entity";
  case FC_STATUS_LOCKED:
    return "Locked";
  case FC_STATUS_FAILED_DEPENDENCY:
    return "Failed Dependency";
  case FC_STATUS_TOO_EARLY:
    return "Too Early";
  case FC_STATUS_UPGRADE_REQUIRED:
    return "Upgrade Required";
  case FC_STATUS_PRECONDITION_REQUIRED:
    return "Precondition Required";
  case FC_STATUS_TOO_MANY_REQUESTS:
    return "Too Many Requests";
  case FC_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE:
    return "Request Header Fields Too Large";
  case FC_STATUS_UNAVAILABLE_FOR_LEGAL_REASONS:
    return "Unavailable For Legal Reasons";

  /* 5xx: Server Errors */
  case FC_STATUS_INTERNAL_SERVER_ERROR:
    return "Internal Server Error";
  case FC_STATUS_NOT_IMPLEMENTED:
    return "Not Implemented";
  case FC_STATUS_BAD_GATEWAY:
    return "Bad Gateway";
  case FC_STATUS_SERVICE_UNAVAILABLE:
    return "Service Unavailable";
  case FC_STATUS_GATEWAY_TIMEOUT:
    return "Gateway Timeout";
  case FC_STATUS_HTTP_VERSION_NOT_SUPPORTED:
    return "HTTP Version Not Supported";
  case FC_STATUS_VARIANT_ALSO_NEGOTIATES:
    return "Variant Also Negotiates";
  case FC_STATUS_INSUFFICIENT_STORAGE:
    return "Insufficient Storage";
  case FC_STATUS_LOOP_DETECTED:
    return "Loop Detected";
  case FC_STATUS_NOT_EXTENDED:
    return "Not Extended";
  case FC_STATUS_NETWORK_AUTH_REQUIRED:
    return "Network Authentication Required";
  }
}

char *fc__http_method_str(fc_http_method method)
{
  switch (method)
  {
  case FC_HTTP_GET:
    return "GET";
  case FC_HTTP_POST:
    return "POST";
  case FC_HTTP_PUT:
    return "PUT";
  case FC_HTTP_DELETE:
    return "DELETE";
  case FC_HTTP_HEAD:
    return "HEAD";
  case FC_HTTP_OPTIONS:
    return "OPTIONS";
  case FC_HTTP_TRACE:
    return "TRACE";
  case FC_HTTP_CONNECT:
    return "CONNECT";
  case FC_HTTP_PATCH:
    return "PATCH";
  case FC__HTTP_METHODS_COUNT:
    return "Unknown method";
  }
}
