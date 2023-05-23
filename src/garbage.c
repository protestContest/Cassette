#include "garbage.h"

static u32 ValSize(Val value)
{
  if (IsTupleHeader(value)) {
    return Max(1, RawVal(value)) + 1;
  } else if (IsBinaryHeader(value)) {
    return NumBinaryCells(RawVal(value)) + 1;
  } else if (IsMapHeader(value)) {
    return 3;
  } else {
    return 1;
  }
}

static Val CopyValue(Val value, Mem *old_mem, Mem *new_mem)
{
  if (IsPair(value) && !IsNil(value)) {
    u32 index = RawVal(value);
    if (Eq(old_mem->values[index], SymbolFor("__moved__"))) {
      return old_mem->values[index+1];
    }

    Val new_val = PairVal(VecCount(new_mem->values));
    VecPush(new_mem->values, old_mem->values[index]);
    VecPush(new_mem->values, old_mem->values[index+1]);
    old_mem->values[index] = SymbolFor("__moved__");
    old_mem->values[index+1] = new_val;
    return new_val;
  } else if (IsObj(value)) {
    u32 index = RawVal(value);
    if (Eq(old_mem->values[index], SymbolFor("__moved__"))) {
      return old_mem->values[index+1];
    }

    Val new_val = ObjVal(VecCount(new_mem->values));
    for (u32 i = 0; i < ValSize(old_mem->values[index]); i++) {
      VecPush(new_mem->values, old_mem->values[index + i]);
    }

    old_mem->values[index] = SymbolFor("__moved__");
    old_mem->values[index+1] = new_val;
    return new_val;
  } else {
    return value;
  }
}

void CollectGarbage(VM *vm)
{
  Mem *mem = vm->mem;

#if DEBUG_GC
  PrintVM(vm);
  PrintMem(mem);
  u32 size = VecCount(mem->values);
#endif

  Val *new_vals = NewVec(Val, VecCount(mem->values) / 2);
  VecPush(new_vals, nil);
  VecPush(new_vals, nil);
  Mem new_mem = {new_vals, mem->symbols};

  u32 scan = VecCount(new_vals);

  for (u32 i = 0; i < NUM_REGS; i++) {
    vm->regs[i] = CopyValue(vm->regs[i], mem, &new_mem);
  }

  while (scan < VecCount(new_vals)) {
    if (IsBinaryHeader(new_mem.values[scan])) {
      scan += NumBinaryCells(RawVal(new_mem.values[scan])) + 1;
    } else {
      new_mem.values[scan] = CopyValue(new_mem.values[scan], mem, &new_mem);
      scan += ValSize(new_mem.values[scan]);
    }
  }

  FreeVec(mem->values);
  mem->values = new_mem.values;

#if DEBUG_GC
  PrintVM(vm);
  PrintMem(mem);

  u32 collected = size - VecCount(mem->values);
  float pct = 100*(float)collected / (float)size;
  Print("Collected ");
  PrintMemSize(collected*sizeof(Val));
  Print(" (");
  PrintFloat(pct, 1);
  Print("%)\n");
#endif
}

#define GC_THRESHHOLD   1024
void MaybeCollectGarbage(VM *vm)
{
  if (VecCount(vm->mem->values) > GC_THRESHHOLD) {
    CollectGarbage(vm);
  }
}
