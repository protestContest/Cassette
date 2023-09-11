#include "source.h"

u32 LineNum(char *src, u32 pos)
{
  u32 line = 0;
  for (; *src != '\0' && pos > 0; src++, pos--) {
    if (*src == '\n') line++;
  }
  return line;
}

char *LineEnd(char *line)
{
  while (*line != '\0' && *line != '\n') line++;
  if (*line == '\n') line++;
  return line;
}

