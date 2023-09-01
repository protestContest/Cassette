#pragma once
#include "heap.h"

u32 Inspect(Val value, Heap *mem);

#ifndef LIBCASSETTE
u32 DebugVal(Val value, Heap *mem);
void PrintMem(Heap *mem);
#endif
