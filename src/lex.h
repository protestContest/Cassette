#pragma once
#include "value.h"
#include "mem.h"
#include "source.h"

typedef struct Token {
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
  Source src;
  u32 start;
  u32 pos;
  u32 line;
  u32 col;
  Mem *mem;
  Literal *literals;
} Lexer;

void InitLexer(Lexer *lexer, Source src, Mem *mem);
Token NextToken(Lexer *lexer);
void PrintToken(Token token);
