#pragma once
#include "mem.h"

#define NumBinaryCells(n) ((n) == 0 ? 1 : ((n) - 1) / sizeof(Val) + 1)

Val MakeBinary(u32 length, Mem *mem);
Val BinaryFrom(void *data, u32 length, Mem *mem);
bool IsBinary(Val binary, Mem *mem);
u32 BinaryLength(Val binary, Mem *mem);
void *BinaryData(Val binary, Mem *mem);
