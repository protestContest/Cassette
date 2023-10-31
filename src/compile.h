#pragma once
#include "chunk.h"
#include "symbols.h"
#include "lex.h"

typedef struct {
  Val env;
  Mem mem;
  Lexer lex;
  Chunk *chunk;
} Compiler;

void InitCompiler(Compiler *c, Chunk *chunk);
void DestroyCompiler(Compiler *c);

Val Compile(char *source, Compiler *c);

void PrintCompileError(Val error, Compiler *c);
