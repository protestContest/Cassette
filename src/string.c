#include "string.h"

u32 StrLen(char *str)
{
  u32 len = 0;
  while (*str++ != '\0') len++;
  return len;
}

u32 NumDigits(i32 num)
{
  u32 digits = 0;
  if (num < 0) {
    num = -num;
    digits = 1;
  }
  while (num /= 10 > 0) digits++;
  return digits;
}

u32 FloatDigits(float num)
{
  u32 digits = 0;
  u32 precision = 10000;

  if (num < 0) {
    num = -num;
    digits = 1;
  }

  num += 0.5 / precision;
  u32 integral = num;
  u32 fraction = (num - integral)*precision;
  if (fraction < 0) {
    return digits + 3;
  } else {
    digits += NumDigits(integral);
    digits += 1;
    for (u32 i = precision / 10; i > 1; i /= 10) {
      if (i > fraction) {
        digits += 1;
      }
    }
    digits += NumDigits(fraction);
  }
  return digits;
}
