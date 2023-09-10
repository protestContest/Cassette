#include "garbage.h"
#include "symbol.h"
#include "binary.h"

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

static Val CopyValue(Val value, Heap *from_space, Heap *to_space)
{
  if (IsNil(value)) return nil;

  if (IsPair(value)) {
    u32 index = RawVal(value);
    if (Eq(from_space->values[index], Moved)) {
      return from_space->values[index+1];
    }

    Val new_val = PairVal(MemSize(to_space));
    VecPush(to_space->values, from_space->values[index]);
    VecPush(to_space->values, from_space->values[index+1]);

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
      VecPush(to_space->values, from_space->values[index + i]);
    }

    from_space->values[index] = Moved;
    from_space->values[index+1] = new_val;
    return new_val;
  } else {
    return value;
  }
}

void CollectGarbage(Val **roots, u32 num_roots, Heap *from_space)
{
  Heap to_space = *from_space;
  to_space.values = NewVec(Val, MemSize(from_space) / 2);
  VecPush(to_space.values, nil);
  VecPush(to_space.values, nil);

  u32 scan = MemSize(&to_space);

  for (u32 r = 0; r < num_roots; r++) {
    for (u32 i = 0; i < VecCount(roots[r]); i++) {
      roots[r][i] = CopyValue(roots[r][i], from_space, &to_space);
    }
  }

  while (scan < VecCount(to_space.values)) {
    if (IsBinaryHeader(to_space.values[scan])) {
      scan += ValSize(to_space.values[scan]);
    } else {
      Val new_val = CopyValue(to_space.values[scan], from_space, &to_space);
      to_space.values[scan] = new_val;
      scan++;
    }
  }

  FreeVec(from_space->values);
  *from_space = to_space;
}
