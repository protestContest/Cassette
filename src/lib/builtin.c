#include "builtin.h"

Val StdHead(VM *vm, Val args)
{
  Val pair = TupleGet(args, 0, &vm->mem);
  if (IsPair(pair)) {
    return Head(pair, &vm->mem);
  } else {
    vm->error = TypeError;
    return nil;
  }
}

Val StdTail(VM *vm, Val args)
{
  Val pair = TupleGet(args, 0, &vm->mem);
  if (IsPair(pair)) {
    return Tail(pair, &vm->mem);
  } else {
    vm->error = TypeError;
    return nil;
  }
}

Val StdError(VM *vm, Val args)
{
  vm->error = RuntimeError;
  if (TupleLength(args, &vm->mem) == 0) {
    return nil;
  } else {
    return TupleGet(args, 1, &vm->mem);
  }
}

Val StdIsNil(VM *vm, Val args)
{
  return BoolVal(IsNil(TupleGet(args, 0, &vm->mem)));
}

Val StdIsNum(VM *vm, Val args)
{
  return BoolVal(IsNum(TupleGet(args, 0, &vm->mem)));
}

Val StdIsInt(VM *vm, Val args)
{
  return BoolVal(IsInt(TupleGet(args, 0, &vm->mem)));
}

Val StdIsSym(VM *vm, Val args)
{
  return BoolVal(IsSym(TupleGet(args, 0, &vm->mem)));
}

Val StdIsPair(VM *vm, Val args)
{
  return BoolVal(IsPair(TupleGet(args, 0, &vm->mem)));
}

Val StdIsTuple(VM *vm, Val args)
{
  return BoolVal(IsTuple(TupleGet(args, 0, &vm->mem), &vm->mem));
}

Val StdIsBinary(VM *vm, Val args)
{
  return BoolVal(IsBinary(TupleGet(args, 0, &vm->mem), &vm->mem));
}

Val StdIsMap(VM *vm, Val args)
{
  return BoolVal(IsMap(TupleGet(args, 0, &vm->mem), &vm->mem));
}

