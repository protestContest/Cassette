#pragma once

/* The compiler takes an AST node and compiles it into a chunk. */

#include "chunk.h"
#include "node.h"
#include "env.h"

val PrimitiveEnv(void);
Chunk Compile(Node node, Env env, val modules);
Chunk CompileDefModule(u32 modnum);
void PrintModules(val modules);
