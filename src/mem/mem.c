#include "mem.h"
#include "univ/system.h"
#include "univ/math.h"
#include "univ/hash.h"
#include "univ/str.h"
#include "symbols.h"

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
  InitVec((Vec*)mem, sizeof(Val), capacity);
  /* Nil is defined as the first pair in memory, which references itself */
  Pair(Nil, Nil, mem);
  mem->roots = roots;
}

void PushMem(Mem *mem, Val value)
{
  mem->items[mem->count++] = value;
}

void PushRoot(Mem *mem, Val value)
{
  if (mem->roots) IntVecPush(mem->roots, value);
}

Val PopRoot(Mem *mem, Val value)
{
  if (mem->roots) return mem->roots->items[--mem->roots->count];
  return value;
}

Val TypeSym(Val value, Mem *mem)
{
  if (IsFloat(value)) return FloatType;
  if (IsInt(value)) return IntType;
  if (IsSym(value)) return SymType;
  if (IsPair(value)) return PairType;
  if (IsTuple(value, mem)) return TupleType;
  if (IsBinary(value, mem)) return BinaryType;
  if (IsMap(value, mem)) return MapType;
  return Nil;
}

char *TypeName(Val type)
{
  switch (type) {
  case FloatType:   return "float";
  case IntType:     return "integer";
  case SymType:     return "symbol";
  case PairType:    return "pair";
  case TupleType:   return "tuple";
  case BinaryType:  return "binary";
  case MapType:     return "map";
  default:          return "?";
  }
}

bool ValEqual(Val v1, Val v2, Mem *mem)
{
  if (IsNum(v1, mem) && IsNum(v2, mem)) {
    return RawNum(v1) == RawNum(v2);
  }

  if (TypeSym(v1, mem) != TypeSym(v2, mem)) return false;

  if (IsSym(v1)) return v1 == v2;
  if (v1 == Nil && v2 == Nil) return true;
  if (IsPair(v1)) {
    return ValEqual(Head(v1, mem), Head(v2, mem), mem)
        && ValEqual(Tail(v1, mem), Tail(v2, mem), mem);
  }
  if (IsTuple(v1, mem)) {
    u32 i;
    if (TupleLength(v1, mem) != TupleLength(v2, mem)) return false;
    for (i = 0; i < TupleLength(v1, mem); i++) {
      if (!ValEqual(TupleGet(v1, i, mem), TupleGet(v2, i, mem), mem)) return false;
    }
    return true;
  }
  if (IsBinary(v1, mem)) {
    u32 i;
    char *data1 = BinaryData(v1, mem);
    char *data2 = BinaryData(v2, mem);
    if (BinaryLength(v1, mem) != BinaryLength(v2, mem)) return false;
    for (i = 0; i < BinaryLength(v1, mem); i++) {
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
void CollectGarbage(Mem *mem)
{
  u32 i;
  Mem new_mem;

  if (!mem->roots) {
    ResizeMem(mem, 2*mem->capacity);
    return;
  }

  InitMem(&new_mem, mem->capacity, mem->roots);

  for (i = 0; i < mem->roots->count; i++) {
    VecRef(mem->roots, i) = CopyValue(VecRef(mem->roots, i), mem, &new_mem);
  }

  i = 2; /* Skip nil */
  while (i < new_mem.count) {
    Val value = VecRef(&new_mem, i);
    if (IsBinaryHeader(value)) {
      i += ObjSize(value); /* skip over binary data, since they aren't values */
    } else {
      VecRef(&new_mem, i) = CopyValue(VecRef(&new_mem, i), mem, &new_mem);
      i++;
    }
  }

  Free(mem->items);
  *mem = new_mem;
}

static Val CopyValue(Val value, Mem *from, Mem *to)
{
  Val new_val = Nil;

  if (value == Nil) return Nil;
  if (!IsObj(value) && !IsPair(value)) return value;

  if (VecRef(from, RawVal(value)) == Moved) {
    return VecRef(from, RawVal(value)+1);
  }

  if (IsPair(value)) {
    new_val = PairVal(to->count);
    PushMem(to, Head(value, from));
    PushMem(to, Tail(value, from));
  } else if (IsObj(value)) {
    u32 i;
    new_val = ObjVal(to->count);
    for (i = 0; i < ObjSize(VecRef(from, RawVal(value))); i++) {
      PushMem(to, VecRef(from, RawVal(value)+i));
    }
  }

  VecRef(from, RawVal(value)) = Moved;
  VecRef(from, RawVal(value)+1) = new_val;
  return new_val;
}

static Val ObjSize(Val value)
{
  if (IsTupleHeader(value)) {
    return Max(2, RawVal(value) + 1);
  } else if (IsBinaryHeader(value)) {
    return Max(2, NumBinCells(RawVal(value)) + 1);
  } else if (IsMapHeader(value)) {
    return Max(2, PopCount(RawVal(value)) + 1);
  } else {
    Assert(false);
  }
}
