#pragma once
#include "value.h"
#include "mem.h"

typedef struct {
  u32 type;
  u32 line;
  u32 col;
  char *lexeme;
  u32 length;
  Val value;
} Token;

struct Lexer;
typedef Token (*NextTokenFn)(struct Lexer *lexer);

typedef struct {
  char *lexeme;
  u32 symbol;
} Literal;

typedef struct {
  char *name;
  char *data;
  u32 length;
} Source;

typedef struct Lexer {
  Source src;
  u32 start;
  u32 pos;
  u32 line;
  u32 col;
  Mem *mem;
  u32 num_literals;
  Literal *literals;
} Lexer;

#define LexPeek(lexer, n) (lexer)->src.data[(lexer)->pos + n]
#define IsWhitespace(c)   ((c) == ' ' || (c) == '\t')
#define IsNewline(c)      ((c) == '\r' || (c) == '\n')
#define IsDigit(c)        ((c) >= '0' && (c) <= '9')
#define IsHexDigit(c)     (IsDigit(c) || ((c) >= 'A' && (c) <= 'F'))
#define IsUppercase(c)    ((c) >= 'A' && (c) <= 'Z')
#define IsLowercase(c)    ((c) >= 'a' && (c) <= 'z')
#define IsAlpha(c)        (IsUpper(c) || IsLower(c))

#define IsErrorToken(t)   ((t).type == (u32)-1)

void InitLexer(Lexer *lexer, u32 num_literals, Literal *literals, Source src, Mem *mem);
Token NextToken(Lexer *lexer);
int PrintToken(Token token);
void PrintTokenPosition(Source src, Token token);
void PrintTokenContext(Source src, Token token, u32 num_lines);
