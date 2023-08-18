#include "tuple.h"

Val MakeTuple(u32 length, Heap *mem)
{
  u32 index = MemSize(mem);
  VecPush(mem->values, TupleHeader(length));
  if (length == 0) {
    VecPush(mem->values, nil);
  } else {
    for (u32 i = 0; i < length; i++) {
      VecPush(mem->values, nil);
    }
  }
  return ObjVal(index);
}

bool IsTuple(Val tuple, Heap *mem)
{
  return IsObj(tuple) && IsTupleHeader(mem->values[RawVal(tuple)]);
}

u32 TupleLength(Val tuple, Heap *mem)
{
  return RawVal(mem->values[RawVal(tuple)]);
}

bool TupleContains(Val tuple, Val value, Heap *mem)
{
  for (u32 i = 0; i < TupleLength(tuple, mem); i++) {
    if (Eq(value, TupleGet(tuple, i, mem))) {
      return true;
    }
  }
  return false;
}

void TupleSet(Val tuple, u32 index, Val value, Heap *mem)
{
  mem->values[RawVal(tuple) + 1 + index] = value;
}

Val TuplePut(Val tuple, u32 index, Val value, Heap *mem)
{
  u32 length = TupleLength(tuple, mem);
  Val new_tuple = MakeTuple(length, mem);
  for (u32 i = 0; i < length; i++) {
    if (i == index) {
      TupleSet(new_tuple, i, value, mem);
    } else {
      TupleSet(new_tuple, i, TupleGet(tuple, i, mem), mem);
    }
  }

  return new_tuple;
}

Val TupleInsert(Val tuple, u32 index, Val value, Heap *mem)
{
  u32 length = TupleLength(tuple, mem) + 1;
  Val new_tuple = MakeTuple(length, mem);
  for (u32 i = 0; i < index; i++) {
    TupleSet(new_tuple, i, TupleGet(tuple, i, mem), mem);
  }
  TupleSet(new_tuple, index, value, mem);
  for (u32 i = index+1; i < length; i++) {
    TupleSet(new_tuple, i, TupleGet(tuple, i - 1, mem), mem);
  }
  return new_tuple;
}

Val TupleGet(Val tuple, u32 index, Heap *mem)
{
  return mem->values[RawVal(tuple) + 1 + index];
}
