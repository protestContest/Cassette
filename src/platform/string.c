#include "string.h"

u32 Strlen(char *str)
{
  u32 len = 0;
  while (*str != '\0') len++;
  return len;
}
