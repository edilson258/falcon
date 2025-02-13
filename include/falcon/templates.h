#ifndef __FALCON_TEMPLATES__
#define __FALCON_TEMPLATES__

char CONTENT_TYPE_PLAIN[] = "text/plain";
char CONTENT_TYPE_HTML[] = "text/html";
char CONTENT_TYPE_JSON[] = "application/json";

char OK_RESPONSE[] = "HTTP/1.1 200 OK\r\n"
                     "Server: Falcon\r\n"
                     "Content-Type: text/plain\r\n"
                     "Content-Length: 2\r\n"
                     "\r\n"
                     "Ok";

char RESPONSE_TEMPLATE[] = "HTTP/1.1 %d %s\r\n"
                           "Server: Falcon\r\n"
                           "Content-Type: %s\r\n"
                           "Content-Length: %zu\r\n"
                           "\r\n"
                           "%s";

char HTTP_RESPONSE_HEADER_TEMPLATE[] = "HTTP/1.1 %d %s\r\n"
                                       "Server: Falcon\r\n"
                                       "Content-Type: %s\r\n"
                                       "Content-Length: %zu\r\n"
                                       "\r\n";

char HTML_TEMPLATE[] = "<!DOCTYPE html>\n"
                       "<html lang=\"en\">\n"
                       "<head>\n"
                       "\t<meta charset=\"UTF-8\">\n"
                       "\t<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                       "\t<title>%s</title>\n"
                       "</head>\n"
                       "<body>\n"
                       "\t%s\n"
                       "</body>\n"
                       "</html>\n";

#endif // !__FALCON_TEMPLATES
