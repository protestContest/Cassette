#include "garbage.h"
#include "binary.h"
#include "list.h"

static u32 ValSize(Val value)
{
  if (IsTupleHeader(value)) {
    return RawVal(value) + 1;
  } else if (IsBinaryHeader(value)) {
    return NumBinaryCells(RawVal(value)) + 1;
  } else if (IsMapHeader(value)) {
    if (RawVal(value) == 0) {
      return 3;
    } else {
      return 2;
    }
  } else {
    return 1;
  }
}

Val CopyValue(Val value, Mem *from_space, Mem *to_space)
{
  if (IsNil(value)) return value;

  if (IsPair(value)) {
    if (Head(value, from_space) == Moved) {
      return Tail(value, from_space);
    } else {
      return Pair(Head(value, from_space), Tail(value, from_space), to_space);
    }
  } else if (IsObj(value)) {
    u32 index = RawVal(value);
    if ((*from_space->values)[index] == Moved) {
      return (*from_space->values)[index+1];
    } else {
      Val new_val = ObjVal(MemSize(to_space));
      u32 i;

      for (i = 0; i < ValSize((*from_space->values)[index]); i++) {
        PushVal(to_space, (*from_space->values)[index + i]);
      }

      (*from_space->values)[index] = Moved;
      (*from_space->values)[index+1] = new_val;
      return new_val;
    }
  } else {
    return value;
  }
}

Mem BeginGC(Mem *from_space)
{
  Mem to_space = *from_space;
  to_space.capacity = MemSize(from_space)/2;
  to_space.count = 0;
  to_space.values = (Val**)NewHandle(to_space.capacity * sizeof(Val));
  PushVal(&to_space, Nil);
  PushVal(&to_space, Nil);
  return to_space;
}

void CollectGarbage(Mem *from_space, Mem *to_space)
{
  u32 scan = 0;

  while (scan < MemSize(to_space)) {
    if (IsBinaryHeader((*to_space->values)[scan])) {
      scan += ValSize((*to_space->values)[scan]);
    } else {
      Val new_val = CopyValue((*to_space->values)[scan], from_space, to_space);
      (*to_space->values)[scan] = new_val;
      scan++;
    }
  }
}
