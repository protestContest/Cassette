#pragma once
#include "result.h"
#include "runtime/chunk.h"

#define ModuleName(mod, mem)    ListGet(NodeExpr(mod, mem), 0, mem)
#define ModuleBody(mod, mem)    ListGet(NodeExpr(mod, mem), 1, mem)
#define ModuleImports(mod, mem) ListGet(NodeExpr(mod, mem), 2, mem)
#define ModuleExports(mod, mem) ListGet(NodeExpr(mod, mem), 3, mem)
#define ModuleFile(mod, mem)    ListGet(NodeExpr(mod, mem), 4, mem)

Result BuildProject(u32 num_files, char **filenames, char *stdlib, Chunk *chunk);

Val MakeModule(Val name, Val body, Val imports, Val exports, Val filename, Mem *mem);
u32 CountExports(Val names, HashMap *modules, Mem *mem);
