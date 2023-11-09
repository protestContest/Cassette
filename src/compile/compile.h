#pragma once
#include "univ/hashmap.h"
#include "mem/mem.h"
#include "result.h"
#include "mem/symbols.h"
#include "runtime/chunk.h"

typedef struct {
  u32 pos;
  Val env;
  char *filename;
  Mem *mem;
  SymbolTable *symbols;
  HashMap *modules;
  Chunk *chunk;
} Compiler;

void InitCompiler(Compiler *c, Mem *mem, SymbolTable *symbols, HashMap *modules, Chunk *chunk);
Result CompileScript(Val module, Compiler *c);
Result CompileModule(Val module, u32 mod_num, Compiler *c);
