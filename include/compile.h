#pragma once

/* Functions for compiling ASTNodes into bytecode */

#include "env.h"
#include "module.h"
#include "result.h"

/* Compiles a module's AST into bytecode (storing it in the module) */
Result CompileModule(Module *module, Module *modules, Env *env);
Chunk *CompileIntro(u32 num_modules);
Chunk *CompileCallMod(u32 id);
