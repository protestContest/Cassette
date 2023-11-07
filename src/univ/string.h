#pragma once

#define IsSpace(c)      ((c) == ' ' || (c) == '\t')
#define IsNewline(c)    ((c) == '\n' || (c) == '\r')
#define IsDigit(c)      ((c) >= '0' && (c) <= '9')
#define IsHexDigit(c)   (IsDigit(c) || ((c) >= 'A' && (c) <= 'F'))
#define IsPrintable(c)  ((c) >= 0x20 && (c) < 0x7F)

#define ANSIRed     "\033[31m"
#define ANSINormal  "\033[0m"

u32 StrLen(char *str);
bool StrEq(char *str1, char *str2);
char HexDigit(u8 n);
char *SkipSpaces(char *str);
char *SkipBlankLines(char *str);
char *LineEnd(char *str);
u32 LineNum(char *str, u32 index);
u32 ColNum(char *str, u32 index);
char *Basename(char *str, char sep);
char *JoinStr(char *str1, char *str2, char joiner);
char *CopyStr(char *str);
