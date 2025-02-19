// Logging helpers

#include <stdarg.h>
#include <stdio.h>

#include "log.h"

int verbose = 0;

void verbose_log(const char *format, ...) {
  if (!verbose)
    return;
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}

void error_log(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}
