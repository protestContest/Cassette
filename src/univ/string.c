#include "string.h"

u32 StrLen(char *str)
{
  u32 length = 0;
  while (*str++ != '\0') length++;
  return length;
}

bool StrEq(char *str1, char *str2)
{
  while (*str1 != '\0' && *str2 != '\0' && *str1 == *str2) {
    ++str1;
    ++str2;
  }

  return *str1 == *str2;
}
