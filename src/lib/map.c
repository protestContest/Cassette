#include "map.h"

Val StdMapKeys(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  if (TupleLength(args, mem) != 1) {
    vm->error = ArgError;
    return SymbolFor("error");
  }
  Val map = TupleGet(args, 0, mem);
  if (!IsMap(map, mem)) {
    vm->error = TypeError;
    return SymbolFor("error");
  }

  return MapKeys(map, vm->mem);
}

Val StdMapValues(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  if (TupleLength(args, mem) != 1) {
    vm->error = ArgError;
    return SymbolFor("error");
  }
  Val map = TupleGet(args, 0, mem);
  if (!IsMap(map, mem)) {
    vm->error = TypeError;
    return SymbolFor("error");
  }

  return MapValues(TupleGet(args, 0, vm->mem), vm->mem);
}

Val StdMapPut(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  if (TupleLength(args, mem) != 3) {
    vm->error = ArgError;
    return SymbolFor("error");
  }
  Val map = TupleGet(args, 0, mem);
  if (!IsMap(map, mem)) {
    vm->error = TypeError;
    return SymbolFor("error");
  }
  Val key = TupleGet(args, 1, mem);
  if (!IsSym(map)) {
    vm->error = TypeError;
    return SymbolFor("error");
  }
  Val value = TupleGet(args, 2, mem);

  return MapPut(map, key, value, mem);
}

Val StdMapDelete(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  if (TupleLength(args, mem) != 2) {
    vm->error = ArgError;
    return SymbolFor("error");
  }
  Val map = TupleGet(args, 0, mem);
  if (!IsMap(map, mem)) {
    vm->error = TypeError;
    return SymbolFor("error");
  }
  Val key = TupleGet(args, 1, mem);
  if (!IsSym(map)) {
    vm->error = TypeError;
    return SymbolFor("error");
  }

  return MapDelete(map, key, mem);
}
