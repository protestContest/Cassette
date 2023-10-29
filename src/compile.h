#pragma once
#include "chunk.h"
#include "symbols.h"
#include "lex.h"

typedef struct {
  Val env;
  Mem mem;
  SymbolTable symbols;
  Lexer lex;
} Compiler;

Chunk *Compile(char *source);
