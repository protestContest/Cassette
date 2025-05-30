#pragma once

#define IsSpace(c)        ((c) == ' ' || (c) == '\t')
#define IsNewline(c)      ((c) == '\n' || (c) == '\r')
#define IsWhitespace(c)   (IsSpace(c) || IsNewline(c))
#define IsDigit(c)        ((c) >= '0' && (c) <= '9')
#define IsUppercase(c)    ((c) >= 'A' && (c) <= 'Z')
#define IsLowercase(c)    ((c) >= 'a' && (c) <= 'z')
#define DownChar(c)       ((c) | ((((c) >> 6) & 1) << 5))
#define UpChar(c)         ((c) & ~((((c) >> 6) & 1) << 5))
#define IsAlpha(c)        (IsUppercase(c) || IsLowercase(c))
#define IsHexDigit(c)     (IsDigit(c) || ((c) >= 'A' && (c) <= 'F'))
#define HexDigit(n)       ((n) < 10 ? (n) + '0' : (n) - 10 + 'A')
#define HexByte(c)        (IsDigit(c) ? (c) - '0' : UpChar(c) - 'A' + 10)
#define IsPrintable(c)    ((c) >= 0x20 && (c) < 0x7F)
#define SkipSpace(str)    do { while (IsWhitespace(*(str))) (str)++; } while (0)

typedef struct {
  char *data;
  u32 len;
} Span;

typedef struct {
  Span head;
  Span tail;
  bool ok;
} Cut;

Span StrSpan(char *start, char *end);
Cut StrCut(Span str, char c);

bool StrEq(char *s1, char *s2);
void Copy(void *src, void *dst, u32 size);
u32 StrLen(char *s);
char *LastIndex(char *s, char c);

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
void WriteBE(i32 num, void *dst);
i32 ReadBE(void *bytes);
i16 ReadShortBE(void *src);
char *TerminateString(char *str);
char *StuffHex(char *hex);
void WriteHex(void *data, u32 len, char *buf);

void Downcase(char *str);

bool Match(char *test, char *str);

bool ParseInt(char **str, i32 base, i32 *num);
bool ParseFloat(char **str, float *num);
