#include "primitives.h"
#include <stdio.h>

static Val VMPrint(u32 num_args, VM *vm);

static PrimitiveDef primitives[] = {
  {"print", &VMPrint}
};

PrimitiveDef *Primitives(void)
{
  return primitives;
}

u32 NumPrimitives(void)
{
  return ArrayCount(primitives);
}

Val DefinePrimitives(Mem *mem)
{
  u32 i;
  Val frame = MakeTuple(NumPrimitives(), mem);
  for (i = 0; i < NumPrimitives(); i++) {
    Val primitive = Pair(Primitive, IntVal(i), mem);
    TupleSet(frame, i, primitive, mem);
  }
  return frame;
}

Val DoPrimitive(Val id, u32 num_args, VM *vm)
{
  return primitives[RawInt(id)].fn(num_args, vm);
}

static Val VMPrint(u32 num_args, VM *vm)
{
  u32 i;

  for (i = 0; i < num_args; i++) {
    Val value = StackPop(vm);
    if (IsNum(value)) {
      PrintVal(value, 0);
    } else if (IsSym(value)) {
      PrintVal(value, &vm->symbols);
    } else if (IsBinary(value, &vm->mem)) {
      u32 len = BinaryLength(value, &vm->mem);
      printf("%*.*s", len, len, (char*)BinaryData(value, &vm->mem));
    } else {
      return Error;
    }
    if (i < num_args-1) printf(" ");
  }
  printf("\n");
  return Ok;
}
