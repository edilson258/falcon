#include <cstdio>
#include <cstring>
#include <stdarg.h>
#include <time.h>

#include "debug.hpp"

namespace fc::debug {

static char datetime_example[] = "2025-10-06 15:44:27";

static void __debug__(const char *level, const char *fmt, va_list args, const char *end) {
  time_t current_time;
  time(&current_time);
  struct tm *local_time = localtime(&current_time);

  size_t datetime_buf_len = strlen(datetime_example);
  char datetime_buf[sizeof(datetime_example)]; // includes null-terminator
  strftime(datetime_buf, datetime_buf_len + 1, "%F %R:%S", local_time);

  fprintf(stdout, "[%s] %s: ", datetime_buf, level);
  vfprintf(stdout, fmt, args);
  fprintf(stdout, "%s", end);
}

void info(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  __debug__("INFO", fmt, args, "\n");
  va_end(args);
}

void warn(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  __debug__("WARN", fmt, args, "\n");
  va_end(args);
}

void error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  __debug__("ERROR", fmt, args, "\r\n");
  va_end(args);
}

} // namespace fc::debug
