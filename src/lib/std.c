#include "std.h"

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
