#pragma once
#include "chunk.h"
#include "lex.h"

typedef struct {
  Lexer lex;
  Val env;
  Mem mem;
} Compiler;

Chunk *Compile(char *source);
