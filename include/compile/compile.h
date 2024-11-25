#pragma once

/* Functions for compiling ASTNodes into bytecode */

#include "compile/env.h"
#include "compile/module.h"
#include "runtime/error.h"
#include "univ/hashmap.h"

typedef struct {
  Env *env;
  Error *error;
  Module *modules; /* vec */
  HashMap *mod_map;
  HashMap alias_map;
  HashMap host_imports;
  u32 mod_id;
  u32 current_mod;
} Compiler;

void InitCompiler(Compiler *c, Module *modules, HashMap *mod_map);
void DestroyCompiler(Compiler *c);
Chunk *Compile(ASTNode *ast, Compiler *c);
