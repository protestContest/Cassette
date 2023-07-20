#pragma once
#include "mem.h"

void FindModules(char *path, HashMap *modules, Mem *mem);
Val LoadModule(char *entry, Mem *mem);
Val SplitModName(Val mod_name, Mem *mem);
