#include "str.h"
#include "system.h"

u32 StrLen(char *str)
{
  u32 len = 0;
  while (*str != 0) {
    str++;
    len++;
  }
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

char HexDigit(u8 n)
{
  if (n < 10) return n + '0';
  else return n - 10 + 'A';
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

char *CopyStr(char *str)
{
  u32 len = StrLen(str);
  char *str2 = Alloc(len+1);
  Copy(str, str2, len);
  str2[len] = 0;
  return str2;
}
