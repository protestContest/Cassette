#pragma once

#include <string.h>

#define IsSpace(c)      ((c) == ' ' || (c) == '\t')
#define IsNewline(c)    ((c) == '\n' || (c) == '\r')
#define IsDigit(c)      ((c) >= '0' && (c) <= '9')
#define IsUppercase(c)  ((c) >= 'A' && (c) <= 'Z')
#define IsLowercase(c)  ((c) >= 'a' && (c) <= 'z')
#define IsAlpha(c)      (IsUppercase(c) || IsLowercase(c))
#define IsHexDigit(c)   (IsDigit(c) || ((c) >= 'A' && (c) <= 'F'))
#define HexDigit(n)     ((n) < 10 ? (n) + '0' : (n) - 10 + 'A')
#define IsPrintable(c)  ((c) >= 0x20 && (c) < 0x7F)

#define ANSIRed         "\033[31m"
#define ANSIUnderline   "\033[4m"
#define ANSINormal      "\033[0m"

#define StrLen(str)         strlen(str)
#define StrEq(s1, s2)       (strcmp(s1, s2) == 0)
#define MemEq(s1, s2, n)    (memcmp(s1, s2, n) == 0)
#define CopyStr(s, n)       strndup(s, n)

#define ReadInt(data)       (((data)[0] << 24) | ((data)[1] << 16) | ((data)[2] << 8) | (data)[3])
#define ReadShort(data)     (((data)[0] << 8) | (data)[1])

char *SkipSpaces(char *str);
char *LineEnd(char *str);
u32 LineNum(char *str, u32 index);
u32 ColNum(char *str, u32 index);
i32 FindString(char *needle, u32 nlen, char *haystack, u32 hlen);
char *JoinStr(char *str1, char *str2, char joiner);
char *StrReplace(char *str, char *find, char *replacement);
u32 ParseInt(char *str);

void WriteInt(u32 n, u8 *data);
void WriteShort(u16 n, u8 *data);
u32 WritePadding(u8 *data, u32 size);
