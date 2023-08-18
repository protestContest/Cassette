#pragma once
#include "heap.h"

Val MakeMap(Heap *mem);
bool IsMap(Val val, Heap *mem);
u32 MapCount(Val map, Heap *mem);
bool MapContains(Val map, Val key, Heap *mem);
Val MapGet(Val map, Val key, Heap *mem);
Val MapPut(Val map, Val key, Val value, Heap *mem);
Val MapFrom(Val keys, Val vals, Heap *mem);

u32 InspectMap(Val map, Heap *mem);
