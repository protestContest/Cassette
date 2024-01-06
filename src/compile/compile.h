#pragma once
#include "env.h"
#include "result.h"
#include "parse.h"
#include "mem/symbols.h"
#include "runtime/chunk.h"
#include "univ/hashmap.h"

typedef struct {
  u32 pos;
  Frame *env;
  char *filename;
  SymbolTable *symbols;
  HashMap *mod_map;
  ObjVec *modules;
  Chunk *chunk;
} Compiler;

void InitCompiler(Compiler *c, SymbolTable *symbols, ObjVec *modules, HashMap *mod_map, Chunk *chunk);
Result CompileModuleFrame(u32 num_modules, Compiler *c);
Result CompileScript(Node *module, Compiler *c);
Result CompileModule(Node *module, u32 mod_num, Compiler *c);
