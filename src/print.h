#pragma once
#include "mem.h"

u32 PrintRawVal(Val value, Mem *mem);
void PrintRawValN(Val value, u32 size, Mem *mem);
u32 PrintVal(Mem *mem, Val value);
void PrintTree(Val tree, Mem *mem);
