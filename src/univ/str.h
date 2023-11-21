#pragma once

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

u32 StrLen(char *str);
bool StrEq(char *str1, char *str2);

void StrCat(char *str1, char *str2, char *dst);
u32 NumDigits(u32 num, u32 base);
u32 NumToStr(u32 num, char *str, u32 base, u32 width);
u32 FloatToStr(float num, char *str, u32 width, u32 precision);

i32 FindString(char *needle, u32 nlen, char *haystack, u32 hlen);
char *SkipSpaces(char *str);
char *SkipBlankLines(char *str);
char *JoinPath(char *str1, char *str2);
char *JoinStr(char *str1, char *str2, char joiner);

char *LineEnd(char *str);
u32 LineNum(char *str, u32 index);
u32 ColNum(char *str, u32 index);
char *Basename(char *str, char sep);
char *CopyStr(char *str, u32 length);
