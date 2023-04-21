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

typedef struct Lexer {
  char *src;
  u32 start;
  u32 pos;
  u32 line;
  u32 col;
  Mem *mem;
  u32 num_literals;
  Literal *literals;
} Lexer;

#define LexPeek(lexer, n) (lexer)->src[(lexer)->pos + n]
#define IsWhitespace(c)   ((c) == ' ' || (c) == '\t')
#define IsNewline(c)      ((c) == '\r' || (c) == '\n')
#define IsDigit(c)        ((c) >= '0' && (c) <= '9')
#define IsHexDigit(c)     (IsDigit(c) || ((c) >= 'A' && (c) <= 'F'))
#define IsUppercase(c)    ((c) >= 'A' && (c) <= 'Z')
#define IsLowercase(c)    ((c) >= 'a' && (c) <= 'z')
#define IsAlpha(c)        (IsUpper(c) || IsLower(c))

#define IsErrorToken(t)   ((t).type == (u32)-1)

void InitLexer(Lexer *lexer, u32 num_literals, Literal *literals, char *src, Mem *mem);
Token NextToken(Lexer *lexer);
int PrintToken(Token token);
void PrintSourceContext(Lexer *lexer, u32 num_lines);
