#include "math.h"

Val MathRandom(VM *vm, Val args)
{
  float n = (float)Random() / (float)MaxUInt;
  return FloatVal(n);
}

Val MathRandInt(VM *vm, Val args)
{
  Heap *mem = &vm->mem;
  i32 min = 0, max;

  if (TupleLength(args, mem) == 1) {
    if (!IsInt(TupleGet(args, 0, mem))) {
      vm->error = TypeError;
      return TupleGet(args, 0, mem);
    }
    max = RawInt(TupleGet(args, 0, mem));
  } else if (TupleLength(args, mem) == 2) {
    if (!IsInt(TupleGet(args, 0, mem))) {
      vm->error = TypeError;
      return TupleGet(args, 0, mem);
    }
    min = RawInt(TupleGet(args, 0, mem));
    if (!IsInt(TupleGet(args, 1, mem))) {
      vm->error = TypeError;
      return TupleGet(args, 1, mem);
    }
    max = RawInt(TupleGet(args, 1, mem));
  } else {
    vm->error = ArgError;
    return nil;
  }

  u32 range = max - min;
  return IntVal(min + (i32)(range*((float)Random() / (float)MaxUInt)));
}

Val MathCeil(VM *vm, Val args)
{
  Heap *mem = &vm->mem;
  if (TupleLength(args, mem) != 1) {
    vm->error = ArgError;
    return nil;
  }
  Val num = TupleGet(args, 0, mem);
  if (IsInt(num)) {
    return IntVal(Ceil(RawInt(num)));
  } else if (IsFloat(num)) {
    return FloatVal(Ceil(RawNum(num)));
  } else {
    vm->error = TypeError;
    return num;
  }
}

Val MathFloor(VM *vm, Val args)
{
  Heap *mem = &vm->mem;
  if (TupleLength(args, mem) != 1) {
    vm->error = ArgError;
    return nil;
  }
  Val num = TupleGet(args, 0, mem);
  if (IsInt(num)) {
    return IntVal(Floor(RawInt(num)));
  } else if (IsFloat(num)) {
    return FloatVal(Floor(RawNum(num)));
  } else {
    vm->error = TypeError;
    return num;
  }
}

Val MathRound(VM *vm, Val args)
{
  Heap *mem = &vm->mem;
  if (TupleLength(args, mem) != 1) {
    vm->error = ArgError;
    return nil;
  }
  Val num = TupleGet(args, 0, mem);
  if (IsInt(num)) {
    return IntVal(Round(RawInt(num)));
  } else if (IsFloat(num)) {
    return FloatVal(Round(RawNum(num)));
  } else {
    vm->error = TypeError;
    return num;
  }
}

Val MathAbs(VM *vm, Val args)
{
  Heap *mem = &vm->mem;
  if (TupleLength(args, mem) != 1) {
    vm->error = ArgError;
    return nil;
  }
  Val num = TupleGet(args, 0, mem);
  if (IsInt(num)) {
    return IntVal(Abs(RawInt(num)));
  } else if (IsFloat(num)) {
    return FloatVal(Abs(RawNum(num)));
  } else {
    vm->error = TypeError;
    return num;
  }
}

Val MathMax(VM *vm, Val args)
{
  Heap *mem = &vm->mem;
  if (TupleLength(args, mem) == 0) {
    vm->error = ArgError;
    return nil;
  }
  Val max = TupleGet(args, 0, mem);

  for (u32 i = 1; i < TupleLength(args, mem); i++) {
    Val arg = TupleGet(args, i, mem);
    if (!IsNum(arg)) {
      vm->error = TypeError;
      return arg;
    }
    if (RawNum(max) < RawNum(arg)) {
      max = arg;
    }
  }

  return max;
}

Val MathMin(VM *vm, Val args)
{
  Heap *mem = &vm->mem;
  if (TupleLength(args, mem) == 0) {
    vm->error = ArgError;
    return nil;
  }
  Val max = TupleGet(args, 0, mem);

  for (u32 i = 1; i < TupleLength(args, mem); i++) {
    Val arg = TupleGet(args, i, mem);
    if (!IsNum(arg)) {
      vm->error = TypeError;
      return arg;
    }
    if (RawNum(max) > RawNum(arg)) {
      max = arg;
    }
  }

  return max;
}

Val MathSin(VM *vm, Val args)
{
  Heap *mem = &vm->mem;
  if (TupleLength(args, mem) != 1) {
    vm->error = ArgError;
    return nil;
  }
  Val arg = TupleGet(args, 0, mem);
  if (!IsNum(arg)) {
    vm->error = TypeError;
    return arg;
  }

  return FloatVal(Sin(RawNum(arg)));
}

Val MathCos(VM *vm, Val args)
{
  Heap *mem = &vm->mem;
  if (TupleLength(args, mem) != 1) {
    vm->error = ArgError;
    return nil;
  }
  Val arg = TupleGet(args, 0, mem);
  if (!IsNum(arg)) {
    vm->error = TypeError;
    return arg;
  }

  return FloatVal(Cos(RawNum(arg)));
}

Val MathTan(VM *vm, Val args)
{
  Heap *mem = &vm->mem;
  if (TupleLength(args, mem) != 1) {
    vm->error = ArgError;
    return nil;
  }
  Val arg = TupleGet(args, 0, mem);
  if (!IsNum(arg)) {
    vm->error = TypeError;
    return arg;
  }

  return FloatVal(Tan(RawNum(arg)));
}

Val MathLn(VM *vm, Val args)
{
  Heap *mem = &vm->mem;
  if (TupleLength(args, mem) != 1) {
    vm->error = ArgError;
    return nil;
  }
  Val arg = TupleGet(args, 0, mem);
  if (!IsNum(arg)) {
    vm->error = TypeError;
    return arg;
  }

  return FloatVal(Log(RawNum(arg)));
}

Val MathExp(VM *vm, Val args)
{
  Heap *mem = &vm->mem;
  if (TupleLength(args, mem) != 1) {
    vm->error = ArgError;
    return nil;
  }
  Val arg = TupleGet(args, 0, mem);
  if (!IsNum(arg)) {
    vm->error = TypeError;
    return arg;
  }

  return FloatVal(Exp(RawNum(arg)));
}

Val MathPow(VM *vm, Val args)
{
  Heap *mem = &vm->mem;
  if (TupleLength(args, mem) != 1) {
    vm->error = ArgError;
    return nil;
  }
  Val base = TupleGet(args, 0, mem);
  if (!IsNum(base)) {
    vm->error = TypeError;
    return base;
  }
  Val exp = TupleGet(args, 1, mem);
  if (!IsNum(exp)) {
    vm->error = TypeError;
    return exp;
  }

  return FloatVal(Pow(RawNum(base), RawNum(exp)));
}

Val MathSqrt(VM *vm, Val args)
{
  Heap *mem = &vm->mem;
  if (TupleLength(args, mem) != 1) {
    vm->error = ArgError;
    return nil;
  }
  Val arg = TupleGet(args, 0, mem);
  if (!IsNum(arg)) {
    vm->error = TypeError;
    return arg;
  }

  return FloatVal(Sqrt(RawNum(arg)));
}

Val MathPi(VM *vm, Val args)
{
  return FloatVal(Pi);
}

Val MathE(VM *vm, Val args)
{
  return FloatVal(E);
}

Val MathInfinity(VM *vm, Val args)
{
  return FloatVal(Infinity);
}

