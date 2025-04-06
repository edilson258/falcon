#pragma once

namespace fc {
namespace templates {

const char OK_RESPONSE[] = "HTTP/1.1 200 OK\r\n"
                           "Server: Falcon\r\n"
                           "Content-Type: text/plain\r\n"
                           "Content-Length: 2\r\n"
                           "\r\n"
                           "Ok";

} // namespace templates
} // namespace fc
