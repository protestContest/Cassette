#include "bit.h"

Val BitAnd(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  if (TupleLength(args, mem) != 2) {
    vm->error = ArgError;
    return nil;
  }

  Val a = TupleGet(args, 0, mem);
  if (!IsInt(a)) {
    vm->error = TypeError;
    return a;
  }
  Val b = TupleGet(args, 1, mem);
  if (!IsInt(b)) {
    vm->error = TypeError;
    return b;
  }

  return IntVal(RawInt(a) & RawInt(b));
}

Val BitOr(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  if (TupleLength(args, mem) != 2) {
    vm->error = ArgError;
    return nil;
  }

  Val a = TupleGet(args, 0, mem);
  if (!IsInt(a)) {
    vm->error = TypeError;
    return a;
  }
  Val b = TupleGet(args, 1, mem);
  if (!IsInt(b)) {
    vm->error = TypeError;
    return b;
  }

  return IntVal(RawInt(a) | RawInt(b));
}

Val BitNot(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  if (TupleLength(args, mem) != 1) {
    vm->error = ArgError;
    return nil;
  }

  Val a = TupleGet(args, 0, mem);
  if (!IsInt(a)) {
    vm->error = TypeError;
    return a;
  }

  return IntVal(~RawInt(a));
}

Val BitXOr(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  if (TupleLength(args, mem) != 2) {
    vm->error = ArgError;
    return nil;
  }

  Val a = TupleGet(args, 0, mem);
  if (!IsInt(a)) {
    vm->error = TypeError;
    return a;
  }
  Val b = TupleGet(args, 1, mem);
  if (!IsInt(b)) {
    vm->error = TypeError;
    return b;
  }

  return IntVal(RawInt(a) ^ RawInt(b));
}

Val BitShift(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  if (TupleLength(args, mem) != 2) {
    vm->error = ArgError;
    return nil;
  }

  Val a = TupleGet(args, 0, mem);
  if (!IsInt(a)) {
    vm->error = TypeError;
    return a;
  }
  Val b = TupleGet(args, 1, mem);
  if (!IsInt(b)) {
    vm->error = TypeError;
    return b;
  }

  return IntVal(RawInt(a) >> RawInt(b));
}

Val BitCount(VM *vm, Val args)
{
  Heap *mem = vm->mem;
  if (TupleLength(args, mem) != 1) {
    vm->error = ArgError;
    return nil;
  }

  Val a = TupleGet(args, 0, mem);
  if (!IsInt(a)) {
    vm->error = TypeError;
    return a;
  }

  return IntVal(PopCount(RawInt(a)));
}

