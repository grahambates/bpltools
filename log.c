#include <stdio.h>
#include <stdarg.h>

#include "log.h"

int verbose = 0;

void verbose_log(const char *format, ...) {
    if (!verbose) return;
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
