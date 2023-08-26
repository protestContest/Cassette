#include "tuple.h"

Val TupleToList(VM *vm, Val args)
{
  Heap *mem = vm->mem;

  Val tuple = TupleGet(args, 0, mem);
  Val list = nil;
  for (u32 i = 0; i < TupleLength(tuple, mem); i++) {
    list = Pair(TupleGet(tuple, i, mem), list, mem);
  }

  return ReverseList(list, mem);
}
