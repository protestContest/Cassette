#include "std.h"
#include "io.h"

static struct {u32 cap; u32 count; PrimitiveDef items[];} stdlib = {
  4, 4, {
    {NULL, "head", StdHead},
    {NULL, "tail", StdTail},
    {"IO", "print", IOPrint},
    {"IO", "inspect", IOInspect}
  }
};

PrimitiveDef *StdLib(void)
{
  return stdlib.items;
}

Val StdHead(VM *vm, Val args)
{
  Val pair = TupleGet(args, 0, vm->mem);
  if (IsPair(pair)) {
    return Head(pair, vm->mem);
  } else {
    vm->error = TypeError;
    return nil;
  }
}

Val StdTail(VM *vm, Val args)
{
  Val pair = TupleGet(args, 0, vm->mem);
  if (IsPair(pair)) {
    return Tail(pair, vm->mem);
  } else {
    vm->error = TypeError;
    return nil;
  }
}
