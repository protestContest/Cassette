#include "map.h"

Val StdMapNew(VM *vm, Val args)
{
  return MakeMap(vm->mem);
}

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
    return map;
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
    return map;
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
    return map;
  }
  Val key = TupleGet(args, 1, mem);
  Val value = TupleGet(args, 2, mem);

  return MapPut(map, HashValue(key, mem), value, mem);
}

Val StdMapGet(VM *vm, Val args)
{  Heap *mem = vm->mem;
  if (TupleLength(args, mem) != 2) {
    vm->error = ArgError;
    return SymbolFor("error");
  }
  Val map = TupleGet(args, 0, mem);
  if (!IsMap(map, mem)) {
    vm->error = TypeError;
    return map;
  }
  Val key = TupleGet(args, 1, mem);
  return MapGet(map, HashValue(key, mem), mem);
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
    return map;
  }
  Val key = TupleGet(args, 1, mem);

  return MapDelete(map, HashValue(key, mem), mem);
}

Val StdMapToList(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  if (TupleLength(args, mem) != 1) {
    vm->error = ArgError;
    return SymbolFor("error");
  }
  Val map = TupleGet(args, 0, mem);
  if (!IsMap(map, mem)) {
    vm->error = TypeError;
    return map;
  }

  Val keys = MapKeys(map, mem);
  Val values = MapValues(map, mem);
  for (u32 i = 0; i < TupleLength(keys, mem); i++) {
    TupleSet(keys, i, Pair(TupleGet(keys, i, mem), TupleGet(values, i, mem), mem), mem);
  }
  return keys;
}
