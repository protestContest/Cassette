#include "garbage.h"
#include "symbol.h"
#include "binary.h"
#include "univ/memory.h"

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

Val CopyValue(Val value, Heap *from_space, Heap *to_space)
{
  if (IsNil(value)) return nil;

  if (IsPair(value)) {
    u32 index = RawVal(value);
    if (Eq(from_space->values[index], Moved)) {
      return from_space->values[index+1];
    }

    Val new_val = PairVal(MemSize(to_space));
    PushVal(to_space, from_space->values[index]);
    PushVal(to_space, from_space->values[index+1]);

    from_space->values[index] = Moved;
    from_space->values[index+1] = new_val;

    return new_val;
  } else if (IsObj(value)) {
    u32 index = RawVal(value);
    if (Eq(from_space->values[index], Moved)) {
      return from_space->values[index+1];
    }

    Val new_val = ObjVal(MemSize(to_space));
    for (u32 i = 0; i < ValSize(from_space->values[index]); i++) {
      PushVal(to_space, from_space->values[index + i]);
    }

    from_space->values[index] = Moved;
    from_space->values[index+1] = new_val;
    return new_val;
  } else {
    return value;
  }
}

Heap BeginGC(Heap *from_space)
{
  Heap to_space = *from_space;
  to_space.capacity = MemSize(from_space)/2;
  to_space.count = 0;
  to_space.values = Allocate(to_space.capacity * sizeof(Val));
  PushVal(&to_space, nil);
  PushVal(&to_space, nil);
  return to_space;
}

void CollectGarbage(Heap *from_space, Heap *to_space)
{
  u32 scan = MemSize(to_space);

  while (scan < to_space->count) {
    if (IsBinaryHeader(to_space->values[scan])) {
      scan += ValSize(to_space->values[scan]);
    } else {
      Val new_val = CopyValue(to_space->values[scan], from_space, to_space);
      to_space->values[scan] = new_val;
      scan++;
    }
  }
}
