#pragma once

#define IsSpace(c)        ((c) == ' ' || (c) == '\t')
#define IsNewline(c)      ((c) == '\n' || (c) == '\r')
#define IsDigit(c)        ((c) >= '0' && (c) <= '9')
#define IsUppercase(c)    ((c) >= 'A' && (c) <= 'Z')
#define IsLowercase(c)    ((c) >= 'a' && (c) <= 'z')
#define IsAlpha(c)        (IsUppercase(c) || IsLowercase(c))
#define IsHexDigit(c)     (IsDigit(c) || ((c) >= 'A' && (c) <= 'F'))
#define HexDigit(n)       ((n) < 10 ? (n) + '0' : (n) - 10 + 'A')
#define IsPrintable(c)    ((c) >= 0x20 && (c) < 0x7F)
#define StrEq(s1, s2)     ((s1) && (s2) && strcmp(s1, s2) == 0)
#define Copy(s, d, c)     memmove(d,s,c)

char *NewString(char *str);
char *StringFrom(char *str, u32 len);
char *FormatString(char *format, char *str);
char *FormatInt(char *format, i32 num);
char *JoinStr(char *a, char *b, char join);
char *StrCat(char *a, char *b);
char *LineStart(u32 index, char *str);
char *LineEnd(u32 index, char *str);
u32 LineNum(char *str, u32 index);
u32 ColNum(char *str, u32 index);
u32 WriteStr(char *str, u32 len, char *buf);
u32 WriteNum(i32 num, char *buf);
