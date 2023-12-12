#include "mem.h"
#include "univ/system.h"

Val MakeTuple(u32 length, Mem *mem)
{
  u32 i;
  Val tuple;

  if (mem->count + length + 1 > mem->capacity) CollectGarbage(mem);
  if (mem->count + length + 1 > mem->capacity) ResizeMem(mem, 2*mem->capacity);
  Assert(mem->count + length + 1 <= mem->capacity);

  tuple = ObjVal(mem->count);
  PushMem(mem, TupleHeader(length));
  for (i = 0; i < length; i++) {
    PushMem(mem, Nil);
  }
  return tuple;
}

bool TupleContains(Val tuple, Val item, Mem *mem)
{
  u32 i;
  for (i = 0; i < TupleLength(tuple, mem); i++) {
    if (TupleGet(tuple, i, mem) == item) return true;
  }

  return false;
}

void TupleSet(Val tuple, u32 i, Val value, Mem *mem)
{
  Assert(i < TupleLength(tuple, mem));
  VecRef(mem, RawVal(tuple) + i + 1) = value;
}

Val TupleCat(Val tuple1, Val tuple2, Mem *mem)
{
  Val result;
  u32 len1, len2, i;
  len1 = TupleLength(tuple1, mem);
  len2 = TupleLength(tuple2, mem);
  result = MakeTuple(len1+len2, mem);
  for (i = 0; i < len1; i++) {
    TupleSet(result, i, TupleGet(tuple1, i, mem), mem);
  }
  for (i = 0; i < len2; i++) {
    TupleSet(result, len1+i, TupleGet(tuple1, i, mem), mem);
  }
  return result;
}
