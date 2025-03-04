#include "univ/str.h"
#include "univ/math.h"
#include <string.h>
#include <malloc/malloc.h>

Span StrSpan(char *start, char *end)
{
  Span span = {0};
  span.data = start;
  span.len = start ? end - start : 0;
  return span;
}

Cut StrCut(Span span, char c)
{
  Cut cut = {0};
  char *start, *end, *data;
  if (span.len == 0) return cut;
  start = span.data;
  end = span.data + span.len;
  data = start;
  while (data < end && *data != c) data++;
  cut.ok = data < end;
  cut.head = StrSpan(start, data);
  cut.tail = StrSpan(data + cut.ok, end);
  return cut;
}

bool StrEq(char *s1, char *s2)
{
  return s1 && s2 && strcmp(s1,s2) == 0;
}

void Copy(void *src, void *dst, u32 size)
{
  memmove(dst, src, size);
}

u32 StrLen(char *s)
{
  u32 len = 0;
  while (*s) {
    if ((*s & 0xC0) != 0x80) len++;
    s++;
  }
  return len;
}

char *LastIndex(char *s, char c)
{
  return strrchr(s, c);
}

char *NewString(char *str)
{
  if (!str) return 0;
  return StringFrom(str, StrLen(str));
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
  len_str = StrLen(str);
  len_format = StrLen(format);

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
  u32 len1 = StrLen(str1), len2 = StrLen(str2);
  u32 joinlen = joiner != 0;
  char *str = malloc(len1 + joinlen + len2 + 1);
  Copy(str1, str, len1);
  if (joiner) str[len1] = joiner;
  Copy(str2, str+len1+1, len2);
  str[len1+1+len2] = 0;
  return str;
}

char *StrCat(char *a, char *b)
{
  u32 len1 = StrLen(a), len2 = StrLen(b);
  char *str = malloc(len1 + len2 + 1);
  Copy(a, str, len1);
  Copy(b, str+len1, len2);
  str[len1 + len2] = 0;
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

void WriteBE(u32 num, void *dst)
{
  u8 *bytes = (u8*)dst;
  bytes[0] = num >> 24;
  bytes[1] = (num >> 16) & 0xFF;
  bytes[2] = (num >> 8) & 0xFF;
  bytes[3] = num & 0xFF;
}

u32 ReadBE(void *src)
{
  u8 *bytes = (u8*)src;
  return
      (((u32)bytes[0] & 0xFF) << 24)
    | (((u32)bytes[1] & 0xFF) << 16)
    | (((u32)bytes[2] & 0xFF) << 8)
    | (((u32)bytes[3] & 0xFF));
}

char *TerminateString(char *str)
{
  i32 size = malloc_size(str);
  str = realloc(str, size+1);
  str[size] = 0;
  return str;
}

char *StuffHex(char *hex)
{
  char *dst = 0;
  u32 len = 0;
  u32 cap = 0;

  while (*hex) {
    u8 byte;
    if (!IsHexDigit(*hex)) {
      hex++;
      continue;
    }

    byte = HexByte(*hex) << 4;
    hex++;
    if (IsHexDigit(*hex)) {
      byte |= HexByte(*hex);
      hex++;
    }

    if (len+1 > cap) {
      cap = Max(2*cap, 1);
      dst = realloc(dst, cap);
    }
    dst[len++] = byte;
  }
  return realloc(dst, len);
}
