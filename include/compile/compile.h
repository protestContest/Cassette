#pragma once

/*
 * A compiler object holds context for compiling a module's AST to a Chunk.
 */

#include "compile/env.h"
#include "compile/project.h"
#include "runtime/error.h"
#include "univ/hashmap.h"

typedef struct {
  Project *project; /* borrowed */
  Error *error; /* borrowed */
  Env *env;
  u32 current_mod;
  HashMap alias_map;
  HashMap host_imports;
} Compiler;

void InitCompiler(Compiler *c, Project *project);
void DestroyCompiler(Compiler *c);
Error *Compile(Compiler *c, Module *mod);
