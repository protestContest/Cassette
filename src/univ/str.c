#include "str.h"
#include "hash.h"
#include "math.h"
#include "system.h"

char *SkipSpaces(char *str)
{
  while (*str == ' ' || *str == '\t') str++;
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

void WriteInt(u32 n, u8 *data)
{
  data[0] = (n >> 24) & 0xFF;
  data[1] = (n >> 16) & 0xFF;
  data[2] = (n >> 8) & 0xFF;
  data[3] = n & 0xFF;
}

void WriteShort(u16 n, u8 *data)
{
  data[0] = (n >> 8) & 0xFF;
  data[1] = n & 0xFF;
}

u32 WritePadding(u8 *data, u32 size)
{
  u32 i;
  u32 n = Align(size, 4) - size;
  for (i = 0; i < n; i++) *data++ = 0;
  return n;
}

