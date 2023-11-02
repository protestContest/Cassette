#pragma once
#include "chunk.h"
#include "symbols.h"
#include "lex.h"
#include "primitives.h"

typedef struct {
  Val env;
  Mem *mem;
  HashMap *modules;
  Chunk *chunk;
  u32 pos;
} Compiler;

typedef struct {
  bool ok;
  char *error;
  u32 pos;
} CompileResult;

void InitCompiler(Compiler *c, Mem *mem, HashMap *modules, Chunk *chunk);
CompileResult CompileModule(Val module, Val env, u32 mod_num, Compiler *c);
void PrintCompileError(CompileResult error, char *filename);
