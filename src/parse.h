#pragma once
#include "value.h"
#include "mem.h"
#include "lex.h"

typedef struct {
  Lexer lex;
  i32 *stack;
  Val *nodes;
  Mem *mem;
} Parser;

Val Parse(Source src, Mem *mem);
Val ParseFile(char *filename, Mem *mem);
Val SyntaxError(Source src, char *message, Token token);
