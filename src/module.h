#pragma once

#include "mem.h"

Val MakeModule(Val name, Val stmts, Val imports, Val exports, Val filename, Mem *mem);
Val ModuleName(Val mod, Mem *mem);
Val ModuleBody(Val mod, Mem *mem);
Val ModuleImports(Val mod, Mem *mem);
Val ModuleExports(Val mod, Mem *mem);
Val ModuleFile(Val mod, Mem *mem);
