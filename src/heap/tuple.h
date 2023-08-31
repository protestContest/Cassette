#pragma once
#include "heap.h"

Val MakeTuple(u32 length, Heap *mem);
bool IsTuple(Val tuple, Heap *mem);
u32 TupleLength(Val tuple, Heap *mem);
bool TupleContains(Val tuple, Val value, Heap *mem);
Val TupleGet(Val tuple, u32 index, Heap *mem);
void TupleSet(Val tuple, u32 index, Val value, Heap *mem);
Val TuplePut(Val tuple, u32 index, Val value, Heap *mem);
Val TupleInsert(Val tuple, u32 index, Val value, Heap *mem);
Val JoinTuples(Val tuple1, Val tuple2, Heap *mem);
