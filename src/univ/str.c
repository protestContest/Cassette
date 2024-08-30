#include "univ/str.h"
#include "univ/hash.h"
#include "univ/math.h"
#include "univ/vec.h"

char *NewString(char *str)
{
  if (!str) return 0;
  return StringFrom(str, strlen(str));
}

char *StringFrom(char *str, u32 len)
{
  char *newstr;
  if (!str) return 0;
  newstr = malloc(len+1);
  Copy(str, newstr, len);
  newstr[len] = 0;
  return newstr;
}

char *FormatString(char *format, char *str)
{
  u32 start;
  u32 len_result;
  u32 len_str;
  u32 len_format;
  if (!str) str = "";
  len_str = strlen(str);
  len_format = strlen(format);

  for (start = 0; format[start]; start++) {
    if (format[start] == '^') break;
  }
  if (start == len_format) return format;
  len_result = len_format - 1 + len_str;

  format = realloc(format, len_result + 1);
  format[len_result] = 0;

  Copy(format + start + 1, format + start + len_str, len_format - start - 1);
  Copy(str, format + start, len_str);
  return format;
}

char *FormatInt(char *format, i32 num)
{
  char numStr[256];
  snprintf(numStr, 256, "%d", num);
  return FormatString(format, numStr);
}

char *JoinStr(char *str1, char *str2, char joiner)
{
  u32 len1 = strlen(str1), len2 = strlen(str2);
  u32 joinlen = joiner != 0;
  char *str = malloc(len1 + joinlen + len2 + 1);
  Copy(str1, str, len1);
  if (joiner) str[len1] = joiner;
  Copy(str2, str+len1+1, len2);
  str[len1+1+len2] = 0;
  return str;
}

char *LineStart(u32 index, char *str)
{
  while (index > 0 && !IsNewline(str[index-1])) index--;
  return str + index;
}

char *LineEnd(u32 index, char *str)
{
  while (str[index] != 0 && !IsNewline(str[index])) index++;
  return str + index + IsNewline(str[index]);
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

u32 WriteStr(char *str, u32 len, char *buf)
{
  u32 buflen = buf ? len+1 : 0;
  return snprintf(buf, buflen, "%*.*s", len, len, str);
}

u32 WriteNum(i32 num, char *buf)
{
  u32 len = buf ? NumDigits(num, 10) + 1 : 0;
  return snprintf(buf, len, "%d", num);
}
