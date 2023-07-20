#pragma once
#include "mem.h"

Val LoadModule(char *entry, Mem *mem);
Val SplitModName(Val mod_name, Mem *mem);
