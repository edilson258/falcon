#pragma once

#include <string>
#include <unordered_map>

static constexpr char http_header_cfmt[] = "HTTP/1.1 %d %s\r\n"
                                           "Server: Falcon\r\n"
                                           "%s\r\n"
                                           "\r\n";

static const std::unordered_map<std::string, std::string> CONTENT_TYPES = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".svg", "image/svg+xml"},
    {".ico", "image/x-icon"},
    {".txt", "text/plain"},
    {".pdf", "application/pdf"},
    {".zip", "application/zip"},
    {".gz", "application/gzip"},
    {".xml", "application/xml"}};
