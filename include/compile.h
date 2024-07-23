#pragma once

/* Functions for compiling ASTNodes into bytecode */

#include "result.h"
#include "parse.h"
#include "env.h"
#include "module.h"

/* Compiles a module's AST into bytecode (storing it in the module) */
Result CompileModule(Module *module, u32 id, Module *modules, Env *env);
Chunk *CompileIntro(Module *modules);
Chunk *CompileCallMod(u32 id);
