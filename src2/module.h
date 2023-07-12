#pragma once
#include "mem.h"

Val LoadModules(char *entry, Mem *mem);
Val WrapModule(Val ast, Val name, Mem *mem);
Val FindImports(Val ast, Mem *mem);
Val FindExports(Val ast, Mem *mem);
