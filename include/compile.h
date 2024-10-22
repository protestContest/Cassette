#pragma once

/* Functions for compiling ASTNodes into bytecode */

#include "env.h"
#include "error.h"
#include "module.h"
#include "univ/hashmap.h"

typedef struct {
  Env *env;
  Error *error;
  Module *modules; /* vec */
  HashMap *mod_map;
  HashMap alias_map;
  u32 mod_id;
} Compiler;

void InitCompiler(Compiler *c, Module *modules, HashMap *mod_map);
void DestroyCompiler(Compiler *c);
Chunk *Compile(ASTNode *ast, Compiler *c);
