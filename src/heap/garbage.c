#include "garbage.h"
#include "symbol.h"
#include "binary.h"

static u32 ValSize(Val value)
{
  if (IsTupleHeader(value)) {
    return RawVal(value) + 1;
  } else if (IsBinaryHeader(value)) {
    return NumBinaryCells(RawVal(value)) + 1;
  } else {
    return 1;
  }
}

static Val CopyValue(Val value, Heap *old_mem, Heap *new_mem)
{
  if (IsNil(value)) return nil;

  if (IsPair(value)) {
    u32 index = RawVal(value);
    if (Eq(old_mem->values[index], SymbolFor("*moved*"))) {
      return old_mem->values[index+1];
    }

    Val new_val = PairVal(MemSize(new_mem));
    VecPush(new_mem->values, old_mem->values[index]);
    VecPush(new_mem->values, old_mem->values[index+1]);

    old_mem->values[index] = SymbolFor("*moved*");
    old_mem->values[index+1] = new_val;

    return new_val;
  } else if (IsObj(value)) {
    u32 index = RawVal(value);
    if (Eq(old_mem->values[index], SymbolFor("*moved*"))) {
      return old_mem->values[index+1];
    }

    Val new_val = ObjVal(MemSize(new_mem));
    for (u32 i = 0; i < ValSize(old_mem->values[index]); i++) {
      VecPush(new_mem->values, old_mem->values[index + i]);
    }

    old_mem->values[index] = SymbolFor("*moved*");
    old_mem->values[index+1] = new_val;
    return new_val;
  } else {
    return value;
  }
}

void CollectGarbage(Val *roots, Heap *mem)
{
  Heap new_mem;
  InitMem(&new_mem, MemSize(mem)/2);
  new_mem.string_map = mem->string_map;
  new_mem.strings = mem->strings;
  MakeSymbol("*moved*", mem);

  u32 scan = MemSize(&new_mem);

  for (u32 i = 0; i < VecCount(roots); i++) {
    roots[i] = CopyValue(roots[i], mem, &new_mem);
  }

  while (scan < VecCount(new_mem.values)) {
    if (IsBinaryHeader(new_mem.values[scan])) {
      scan += ValSize(new_mem.values[scan]);
    } else {
      Val new_val = CopyValue(new_mem.values[scan], mem, &new_mem);
      new_mem.values[scan] = new_val;
      scan++;
    }
  }

  DestroyMem(mem);
  *mem = new_mem;
}
