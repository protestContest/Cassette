#pragma once
#include "runtime/chunk.h"
#include "mem/symbols.h"
#include "lex.h"
#include "result.h"
#include "runtime/primitives.h"

typedef struct {
  Val env;
  Mem *mem;
  HashMap *modules;
  Chunk *chunk;
  u32 pos;
  SymbolTable *symbols;
} Compiler;

void InitCompiler(Compiler *c, Mem *mem, SymbolTable *symbols, HashMap *modules, Chunk *chunk);
BuildResult CompileModule(Val module, Val env, u32 mod_num, Compiler *c);
