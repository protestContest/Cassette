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
char *GrammarSymbolName(u32 sym);
