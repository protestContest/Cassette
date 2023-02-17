#include "error.h"
#include <stdarg.h>
#include <stdio.h>

void Fatal(char *msg, ...)
{
  va_list args;
  va_start(args, msg);
  vprintf(msg, args);
  va_end(args);
}
