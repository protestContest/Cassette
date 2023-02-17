#include "console.h"
#include <stdarg.h>
#include <stdio.h>

int Print(char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  u32 written = vprintf(fmt, args);
  va_end(args);
  return written;
}

char *ReadLine(char *buf, u32 size)
{
  return fgets(buf, size, stdin);
}
