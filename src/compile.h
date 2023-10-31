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

typedef struct {
  bool ok;
  char *error;
  u32 pos;
} CompileResult;

void InitCompiler(Compiler *c, Chunk *chunk);
void DestroyCompiler(Compiler *c);

CompileResult Compile(char *source, Compiler *c);
void PrintCompileError(CompileResult error, Compiler *c);
