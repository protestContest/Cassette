#pragma once
#include "heap.h"

#define NumBinaryCells(n) ((n) == 0 ? 1 : ((n) - 1) / sizeof(Val) + 1)

Val MakeBinary(u32 length, Heap *mem);
Val BinaryFrom(void *data, u32 length, Heap *mem);
bool IsBinary(Val binary, Heap *mem);
u32 BinaryLength(Val binary, Heap *mem);
void *BinaryData(Val binary, Heap *mem);
u8 BinaryGetByte(Val binary, u32 i, Heap *mem);
void BinarySetByte(Val binary, u32 i, u8 b, Heap *mem);

Val BinaryAfter(Val binary, u32 index, Heap *mem);
Val BinaryTrunc(Val binary, u32 index, Heap *mem);
Val SliceBinary(Val binary, u32 start, u32 end, Heap *mem);
Val JoinBinaries(Val b1, Val b2, Heap *mem);
