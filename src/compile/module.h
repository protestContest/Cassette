#pragma once

#include "mem/mem.h"
#include "mem/symbols.h"

Val MakeModule(Val name, Val body, Val imports, Val exports, Val filename, Mem *mem);
Val ModuleName(Val mod, Mem *mem);
Val ModuleBody(Val mod, Mem *mem);
Val ModuleImports(Val mod, Mem *mem);
Val ModuleExports(Val mod, Mem *mem);
Val ModuleFile(Val mod, Mem *mem);
u32 CountExports(Val names, HashMap *modules, Mem *mem);
