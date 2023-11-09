#pragma once

#include "mem/mem.h"
#include "univ/hashmap.h"

#define ModuleName(mod, mem) TupleGet(mod, 0, mem)
#define ModuleBody(mod, mem) TupleGet(mod, 1, mem)
#define ModuleImports(mod, mem) TupleGet(mod, 2, mem)
#define ModuleExports(mod, mem) TupleGet(mod, 3, mem)
#define ModuleFile(mod, mem) TupleGet(mod, 4, mem)

Val MakeModule(Val name, Val body, Val imports, Val exports, Val filename, Mem *mem);
u32 CountExports(Val names, HashMap *modules, Mem *mem);
