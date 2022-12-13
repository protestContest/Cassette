#include "string.h"

u32 StringLength(char *src, u32 start, u32 end)
{
  u32 len = 0;
  for (u32 i = start; i < end; i++) {
    if ((src[i] & 0xC0) != 0x80) len++;
  }
  return len;
}
