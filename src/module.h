#pragma once
#include "mem.h"

Val LoadModule(char *entry, Mem *mem);
Val FindImports(Val ast, char *folder, Mem *mem);
Val FindExports(Val ast, Mem *mem);

void FindModules(void);
