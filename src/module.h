#pragma once
#include "mem.h"

void FindModules(char *path, HashMap *modules, Mem *mem);
Val LoadModule(char *entry, char *module_path, Mem *mem);
Val SplitModName(Val mod_name, Mem *mem);
Val LoadError(Val name, Mem *mem);
