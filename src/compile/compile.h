#pragma once
#include "env.h"
#include "parse.h"
#include "runtime/chunk.h"
#include "runtime/mem/symbols.h"
#include "univ/hashmap.h"
#include "univ/result.h"

typedef struct {
  u32 pos;
  Frame *env;
  char *filename;
  SymbolTable *symbols;
  HashMap *mod_map;
  ObjVec *modules;
  Chunk *chunk;
  u32 mod_num;
} Compiler;

void InitCompiler(Compiler *c, SymbolTable *symbols, ObjVec *modules, HashMap *mod_map, Chunk *chunk);
Result CompileModuleFrame(u32 num_modules, Compiler *c);
Result CompileModule(Node *module, Compiler *c);
