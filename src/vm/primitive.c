#include "primitive.h"

Val PrimHead(VM *vm)
{
  Val pair = StackPop(vm);
  if (IsPair(pair)) {
    return Head(pair, vm->mem);
  } else {
    vm->error = TypeError;
    return nil;
  }
}

Val PrimTail(VM *vm)
{
  Val pair = StackPop(vm);
  if (IsPair(pair)) {
    return Tail(pair, vm->mem);
  } else {
    vm->error = TypeError;
    return nil;
  }
}
