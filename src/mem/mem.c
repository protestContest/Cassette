#include "mem.h"
#include "symbols.h"
#include "univ/hash.h"
#include "univ/math.h"
#include "univ/str.h"
#include "univ/system.h"

Val FloatVal(float num)
{
  /* C is weird and I hate it */
  union {Val as_v; float as_f;} convert;
  convert.as_f = num;
  return convert.as_v;
}

float RawFloat(Val value)
{
  union {Val as_v; float as_f;} convert;
  convert.as_v = value;
  return convert.as_f;
}

void InitMem(Mem *mem, u32 capacity, IntVec *roots)
{
  InitVec(mem, sizeof(Val), capacity);
  /* Nil is defined as the first pair in memory, which references itself */
  Pair(Nil, Nil, mem);
  mem->roots = roots;
}

void PushMem(Mem *mem, Val value)
{
  mem->items[mem->count++] = value;
}

bool ValEqual(Val v1, Val v2, Mem *mem)
{
  if (IsNum(v1, mem) && IsNum(v2, mem)) return RawNum(v1) == RawNum(v2);
  if (TypeOf(v1) != TypeOf(v2)) return false;
  if (IsSym(v1)) return v1 == v2;
  if (v1 == Nil && v2 == Nil) return true;

  if (IsPair(v1)) {
    return ValEqual(Head(v1, mem), Head(v2, mem), mem)
        && ValEqual(Tail(v1, mem), Tail(v2, mem), mem);
  }

  if (IsTuple(v1, mem)) {
    u32 i;
    if (TupleCount(v1, mem) != TupleCount(v2, mem)) return false;
    for (i = 0; i < TupleCount(v1, mem); i++) {
      if (!ValEqual(TupleGet(v1, i, mem), TupleGet(v2, i, mem), mem)) return false;
    }
    return true;
  }

  if (IsBinary(v1, mem)) {
    u32 i;
    char *data1 = BinaryData(v1, mem);
    char *data2 = BinaryData(v2, mem);
    if (BinaryCount(v1, mem) != BinaryCount(v2, mem)) return false;
    for (i = 0; i < BinaryCount(v1, mem); i++) {
      if (data1[i] != data2[i]) return false;
    }
    return true;
  }

  if (IsMap(v1, mem)) {
    if (MapCount(v1, mem) != MapCount(v2, mem)) return false;
    return MapIsSubset(v1, v2, mem);
  }

  return false;
}

/* Cheney's algorithm */
static Val CopyValue(Val value, Mem *from, Mem *to);
static Val ObjSize(Val value);

#define Moved             0x7FDE7580 /* *moved* */

void CollectGarbage(Mem *mem)
{
  u32 i;
  Mem new_mem;

  /* printf("GARBAGE DAY!!!\n"); */
  ResizeMem(mem, 2*mem->capacity);
  return;

  InitMem(&new_mem, mem->capacity, mem->roots);

  for (i = 0; i < mem->roots->count; i++) {
    VecRef(mem->roots, i) = CopyValue(VecRef(mem->roots, i), mem, &new_mem);
  }

  i = 2; /* Skip nil */
  while (i < new_mem.count) {
    Val value = MemRef(&new_mem, i);
    if (IsBinaryHeader(value)) {
      i += ObjSize(value); /* skip over binary data, since they aren't values */
    } else {
      MemRef(&new_mem, i) = CopyValue(MemRef(&new_mem, i), mem, &new_mem);
      i++;
    }
  }

  Free(mem->items);
  *mem = new_mem;

  if (mem->count > 0.75*mem->capacity) {
    ResizeMem(mem, 2*mem->capacity);
  } else if (mem->count < 0.25*mem->capacity) {
    ResizeMem(mem, 0.5*mem->capacity);
  }
}

static Val CopyValue(Val value, Mem *from, Mem *to)
{
  Val new_val = Nil;

  if (value == Nil) return Nil;
  if (!IsObj(value) && !IsPair(value)) return value;
  if (IsPrimitive(value)) return value;

  if (MemRef(from, RawVal(value)) == Moved) {
    return MemRef(from, RawVal(value)+1);
  }

  if (IsPair(value)) {
    new_val = PairVal(to->count);
    PushMem(to, Head(value, from));
    PushMem(to, Tail(value, from));
  } else if (IsObj(value)) {
    u32 i;
    new_val = ObjVal(to->count);
    for (i = 0; i < ObjSize(MemRef(from, RawVal(value))); i++) {
      PushMem(to, MemRef(from, RawVal(value)+i));
    }
  }

  MemRef(from, RawVal(value)) = Moved;
  MemRef(from, RawVal(value)+1) = new_val;
  return new_val;
}

static Val ObjSize(Val value)
{
  if (IsTupleHeader(value)) {
    return Max(2, RawVal(value) + 1);
  } else if (IsBinaryHeader(value)) {
    return Max(2, NumBinCells(RawVal(value)) + 1);
  } else if (IsMapHeader(value)) {
    return Max(2, (1 + PopCount(value & 0xFFFF)));
  } else if (IsFuncHeader(value)) {
    return 3;
  } else {
    Assert(false);
  }
}
