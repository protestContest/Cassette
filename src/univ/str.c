#include "str.h"
#include "hash.h"
#include "math.h"
#include "system.h"

u32 StrLen(char *str)
{
  u32 len = 0;
  while (*str++ != 0) len++;
  return len;
}

bool StrEq(char *str1, char *str2)
{
  while (*str1 != '\0' && *str2 != '\0' && *str1 == *str2) {
    ++str1;
    ++str2;
  }

  return *str1 == *str2;
}

bool MemEq(u8 *str1, u8 *str2, u32 size)
{
  u32 i;
  for (i = 0; i < size; i++) {
    if (*str1 != *str2) return false;
  }
  return true;
}

void StrCat(char *str1, char *str2, char *dst)
{
  if (str1 != dst) {
    while (*str1 != 0) *dst++ = *str1++;
  } else {
    while (*str1 != 0) dst++;
  }
  while (*str2 != 0) *dst++ = *str2++;
  *dst = 0;
}

u32 NumDigits(u32 num, u32 base)
{
  u32 len = 0;
  if (num == 0) return 1;

  while (num > 0) {
    len++;
    num /= base;
  }
  return len;
}

/* Prints a number into a string, right-aligned. Returns the number of
characters printed. */
u32 NumToStr(u32 num, char *str, u32 base, u32 width)
{
  u32 i;
  if (width == 0) width = NumDigits(num, base);
  str[width] = 0;
  for (i = 0; i < width; i++) {
    u32 d = num % base;
    str[width-1-i] = HexDigit(d);
    num /= base;
    if (num == 0) return i+1;
  }
  return width;
}

u32 FloatToStr(float num, char *str, u32 width, u32 precision)
{
  u32 i, exp = 1;
  u32 whole = Floor(Abs(num));
  u32 frac, written;
  for (i = 0; i < precision; i++) exp *= 10;
  frac = (num - (float)whole) * exp;
  if (width == 0) width = NumDigits(whole, 10) + NumDigits(frac, 10) + 1 + (num < 0);
  written = NumToStr(frac, str, 10, 0);
  str[width-written-1] = '.';
  written += 1 + NumToStr(whole, str, 10, width - written - 1);
  if (num < 0 && written < width) {
    str[width-written-1] = '-';
    written++;
  }
  return written;
}

char *SkipSpaces(char *str)
{
  while (*str == ' ' || *str == '\t') str++;
  return str;
}

char *SkipBlankLines(char *str)
{
  str = SkipSpaces(str);
  while (IsNewline(*str)) {
    str++;
    str = SkipSpaces(str);
  }
  return str;
}

char *LineEnd(char *str)
{
  while (*str != 0 && !IsNewline(*str)) str++;
  return str;
}

u32 LineNum(char *source, u32 pos)
{
  char *cur = source;
  u32 line = 0;

  while (cur < source + pos) {
    if (*cur == '\n') line++;
    cur++;
  }

  return line;
}

u32 ColNum(char *source, u32 pos)
{
  char *cur = source + pos;

  while (cur > source && *(cur-1) != '\n') cur--;

  return source + pos - cur;
}

i32 FindString(char *needle, u32 nlen, char *haystack, u32 hlen)
{
  /* Rabin-Karp algorithm */
  u32 i, j, item_hash, test_hash;
  if (nlen > hlen) return -1;
  if (nlen == 0 || hlen == 0) return false;

  item_hash = Hash(needle, nlen);
  test_hash = Hash(haystack, nlen);

  for (i = 0; i < hlen - nlen; i++) {
    if (test_hash == item_hash) break;
    test_hash = SkipHash(test_hash, haystack[i], nlen);
    test_hash = AppendHash(test_hash, haystack[i+nlen]);
  }

  if (test_hash == item_hash) {
    for (j = 0; j < nlen; j++) {
      if (haystack[i+j] != needle[j]) return -1;
    }
    return i;
  }
  return -1;
}

char *Basename(char *str, char sep)
{
  char *cur = str, *end = str;
  while (*cur != 0) {
    if (*cur == sep) end = cur;
    cur++;
  }

  cur = Alloc(end - str + 1);
  cur[end-str] = 0;
  Copy(str, cur, end-str);
  return cur;
}

char *JoinPath(char *str1, char *str2)
{
  u32 len1 = StrLen(str1), len2 = StrLen(str2);
  char *str = Alloc(len1 + 1 + len2 + 1);
  while (len1 > 0 && str1[len1-1] == '/') len1--;
  while (len2 > 0 && *str2 == '/') {
    str2++;
    len2--;
  }

  if (len1 > 0) {
    Copy(str1, str, len1);
    str[len1] = '/';
    Copy(str2, str+len1+1, len2);
    str[len1+1+len2] = 0;
  } else {
    Copy(str2, str, len2);
    str[len2] = 0;
  }

  return str;
}

char *JoinStr(char *str1, char *str2, char joiner)
{
  u32 len1 = StrLen(str1), len2 = StrLen(str2);
  char *str = Alloc(len1 + 1 + len2 + 1);
  Copy(str1, str, len1);
  str[len1] = joiner;
  Copy(str2, str+len1+1, len2);
  str[len1+1+len2] = 0;
  return str;
}

char *CopyStr(char *str, u32 length)
{
  char *str2 = Alloc(length+1);
  Copy(str, str2, length);
  str2[length] = 0;
  return str2;
}

char *StrReplace(char *str, char *find, char *replacement)
{
  u32 len = StrLen(str);
  u32 find_len = StrLen(find);
  u32 rep_len = StrLen(replacement);
  u32 new_len = len - find_len + rep_len;
  char *newstr;
  i32 index = FindString(find, find_len, str, len);
  if (index < 0) return str;

  newstr = Alloc(new_len + 1);
  Copy(str, newstr, index);
  Copy(replacement, newstr + index, rep_len);
  Copy(str + index + find_len, newstr + index + rep_len, len - find_len - index);
  newstr[new_len] = 0;
  return newstr;
}

u32 ParseInt(char *str)
{
  u32 num = 0;
  while (IsDigit(*str)) {
    u32 d = *str - '0';
    num = num * 10 + d;
    str++;
  }
  return num;
}
