#include "tuple.h"

Val MakeTuple(u32 length, Mem *mem)
{
  u32 index = MemSize(mem);
  PushVal(mem, TupleHeader(length));
  if (length == 0) {
    PushVal(mem, Nil);
  } else {
    u32 i;
    for (i = 0; i < length; i++) PushVal(mem, Nil);
  }
  return ObjVal(index);
}

bool IsTuple(Val tuple, Mem *mem)
{
  return IsObj(tuple) && IsTupleHeader((*mem->values)[RawVal(tuple)]);
}

u32 TupleLength(Val tuple, Mem *mem)
{
  return RawVal((*mem->values)[RawVal(tuple)]);
}

bool TupleContains(Val tuple, Val value, Mem *mem)
{
  u32 i;
  for (i = 0; i < TupleLength(tuple, mem); i++) {
    if (value == TupleGet(tuple, i, mem)) return true;
  }
  return false;
}

Val TupleGet(Val tuple, u32 index, Mem *mem)
{
  return (*mem->values)[RawVal(tuple) + 1 + index];
}

void TupleSet(Val tuple, u32 index, Val value, Mem *mem)
{
  (*mem->values)[RawVal(tuple) + 1 + index] = value;
}
