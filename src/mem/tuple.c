#include "mem.h"
#include "univ/system.h"

Val MakeTuple(u32 length, Mem *mem)
{
  u32 i;
  Val tuple;

  Assert(CheckMem(mem, length + 1));

  tuple = ObjVal(MemNext(mem));
  PushMem(mem, TupleHeader(length));
  for (i = 0; i < length; i++) {
    PushMem(mem, Nil);
  }
  return tuple;
}

void TupleSet(Val tuple, u32 i, Val value, Mem *mem)
{
  Assert(i < TupleCount(tuple, mem));
  VecRef(mem, RawVal(tuple) + i + 1) = value;
}

bool TupleContains(Val tuple, Val item, Mem *mem)
{
  u32 i;
  for (i = 0; i < TupleCount(tuple, mem); i++) {
    if (TupleGet(tuple, i, mem) == item) return true;
  }

  return false;
}

Val TupleCat(Val tuple1, Val tuple2, Mem *mem)
{
  Val result;
  u32 len1, len2, i;
  len1 = TupleCount(tuple1, mem);
  len2 = TupleCount(tuple2, mem);
  result = MakeTuple(len1+len2, mem);
  for (i = 0; i < len1; i++) {
    TupleSet(result, i, TupleGet(tuple1, i, mem), mem);
  }
  for (i = 0; i < len2; i++) {
    TupleSet(result, len1+i, TupleGet(tuple1, i, mem), mem);
  }
  return result;
}
