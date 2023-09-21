#pragma once
#include "mem.h"

Mem BeginGC(Mem *from_space);
Val CopyValue(Val value, Mem *from_space, Mem *to_space);
void CollectGarbage(Mem *from_space, Mem *to_space);
