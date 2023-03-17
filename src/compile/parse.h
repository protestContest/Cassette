#pragma once
#include "lex.h"

typedef enum {
  Number,
  Identifier,
  Value,
  Primary,
  Sum,
  Product,
  Expr,
} ParseSymbol;
#define NUM_PARSE_SYM 7

typedef struct ASTNode {
  ParseSymbol symbol;
  Token token;
  struct ASTNode *children;
} ASTNode;

typedef struct {
  u32 id;
  Token token;
} ParseState;

typedef struct {
  Lexer lex;
  Token next_token;
  ParseState *stack;
} Parser;

bool Parse(char *src);
