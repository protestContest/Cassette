#pragma once
#include "mem.h"

Val MakeTuple(u32 length, Mem *mem);
bool IsTuple(Val tuple, Mem *mem);
u32 TupleLength(Val tuple, Mem *mem);
bool TupleContains(Val tuple, Val value, Mem *mem);
Val TupleGet(Val tuple, u32 index, Mem *mem);
void TupleSet(Val tuple, u32 index, Val value, Mem *mem);
