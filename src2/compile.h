#pragma once
#include "chunk.h"
#include "lex.h"

typedef struct {
  Token token;
  char **source;
  Chunk *chunk;
  u32 line;
  u32 col;
  bool error;
} Compiler;

void InitCompiler(Compiler *compiler, Chunk *chunk, char **source);
void CompileExpr(Compiler *compiler, u32 precedence);
// Chunk Compile(char *source);
