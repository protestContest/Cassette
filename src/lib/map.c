#include "map.h"

Val StdMapKeys(VM *vm, Val args)
{
  return MapKeys(TupleGet(args, 0, vm->mem), vm->mem);
}

Val StdMapValues(VM *vm, Val args)
{
  return MapValues(TupleGet(args, 0, vm->mem), vm->mem);
}
