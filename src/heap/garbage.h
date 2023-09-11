#pragma once
#include "heap.h"

Heap BeginGC(Heap *from_space);
Val CopyValue(Val value, Heap *from_space, Heap *to_space);
void CollectGarbage(Heap *from_space, Heap *to_space);
