#pragma once

namespace fc {
namespace templates {

constexpr char HTTP_HEADER_CHUNCKED[] = "HTTP/1.1 {} {}\r\n"
                                        "Server: Falcon\r\n"
                                        "Content-Type: {}\r\n"
                                        "Transfer-Encoding: chunked\r\n"
                                        "\r\n";

constexpr char OK_RESPONSE[] = "HTTP/1.1 {} {}\r\n"
                               "Server: Falcon\r\n"
                               "Content-Type: {}\r\n"
                               "Content-Length: {}\r\n"
                               "\r\n"
                               "{}";

constexpr char HTTP_RESPONSE_FORMAT[] = "HTTP/1.1 {} {}\r\n"
                                        "Server: Falcon\r\n"
                                        "Content-Type: {}\r\n"
                                        "Content-Length: {}\r\n"
                                        "\r\n"
                                        "{}";

} // namespace templates
} // namespace fc
