#include "primitives.h"
#include "env.h"
#include "value.h"

typedef Val (*PrimitiveFn)(Val args, VM *vm);

typedef struct {
  char *name;
  PrimitiveFn fn;
} Primitive;

static Val TypeOfOp(Val args, VM *vm);
static Val IsNilOp(Val args, VM *vm);
static Val IsIntegerOp(Val args, VM *vm);
static Val IsFloatOp(Val args, VM *vm);
static Val IsNumericOp(Val args, VM *vm);
static Val IsSymbolOp(Val args, VM *vm);
static Val IsPairOp(Val args, VM *vm);
static Val IsTupleOp(Val args, VM *vm);
// static Val IsMapOp(Val args, VM *vm);
static Val IsBinaryOp(Val args, VM *vm);
static Val IsTrueOp(Val args, VM *vm);
static Val NotOp(Val args, VM *vm);
static Val CeilOp(Val args, VM *vm);
static Val FloorOp(Val args, VM *vm);
static Val RoundOp(Val args, VM *vm);
static Val DivOp(Val args, VM *vm);
static Val RemOp(Val args, VM *vm);
static Val AbsOp(Val args, VM *vm);
static Val MinOp(Val args, VM *vm);
static Val MaxOp(Val args, VM *vm);
static Val RandomOp(Val args, VM *vm);
static Val RandomIntOp(Val args, VM *vm);
static Val PairOp(Val args, VM *vm);
static Val HeadOp(Val args, VM *vm);
static Val TailOp(Val args, VM *vm);
static Val SetHeadOp(Val args, VM *vm);
static Val SetTailOp(Val args, VM *vm);
static Val TupleOp(Val args, VM *vm);
static Val TupleSizeOp(Val args, VM *vm);
static Val GetNthOp(Val args, VM *vm);
static Val SetNthOp(Val args, VM *vm);
static Val MapOp(Val args, VM *vm);
static Val MapSizeOp(Val args, VM *vm);
static Val MapGetOp(Val args, VM *vm);
static Val MapSetOp(Val args, VM *vm);
static Val AllocateOp(Val args, VM *vm);
static Val ByteSizeOp(Val args, VM *vm);
static Val GetByteOp(Val args, VM *vm);
static Val SetByteOp(Val args, VM *vm);
static Val ToStringOp(Val args, VM *vm);
static Val PrintOp(Val args, VM *vm);
static Val EvalOp(Val args, VM *vm);
static Val ApplyOp(Val args, VM *vm);

static Primitive primitives[] = {
  {"type-of", TypeOfOp},
  {"nil?", IsNilOp},
  {"integer?", IsIntegerOp},
  {"float?", IsFloatOp},
  {"numeric?", IsNumericOp},
  {"symbol?", IsSymbolOp},
  {"pair?", IsPairOp},
  {"tuple?", IsTupleOp},
  // {"map?", IsMapOp},
  {"binary?", IsBinaryOp},
  {"true?", IsTrueOp},
  {"not", NotOp},
  {"ceil", CeilOp},
  {"floor", FloorOp},
  {"round", RoundOp},
  {"div", DivOp},
  {"rem", RemOp},
  {"abs", AbsOp},
  {"min", MinOp},
  {"max", MaxOp},
  {"random", RandomOp},
  {"random-int", RandomIntOp},
  {"pair", PairOp},
  {"head", HeadOp},
  {"tail", TailOp},
  {"set-head!", SetHeadOp},
  {"set-tail!", SetTailOp},
  {"tuple", TupleOp},
  {"tuple-size", TupleSizeOp},
  {"get-nth", GetNthOp},
  {"set-nth!", SetNthOp},
  {"map", MapOp},
  {"map-size", MapSizeOp},
  {"map-get", MapGetOp},
  {"map-set!", MapSetOp},
  {"allocate", AllocateOp},
  {"byte-size", ByteSizeOp},
  {"get-byte", GetByteOp},
  {"set-byte!", SetByteOp},
  {"to-string", ToStringOp},
  {"print", PrintOp},
  {"apply", ApplyOp},
  {"eval", EvalOp},
};

Val DoPrimitive(Val op, Val args, VM *vm)
{
  for (u32 i = 0; i < ArrayCount(primitives); i++) {
    if (Eq(MakeSymbol(vm->mem, primitives[i].name), op)) {
      return primitives[i].fn(args, vm);
    }
  }

  return RuntimeError("Unknown primitive", op, vm);
}

static Val TypeOfOp(Val args, VM *vm)
{
  Val arg = First(vm->mem, args);
  if (IsInt(arg)) return MakeSymbol(vm->mem, "integer");
  if (IsNum(arg)) return MakeSymbol(vm->mem, "float");
  if (IsSym(arg)) return MakeSymbol(vm->mem, "symbol");
  if (IsPair(arg)) return MakeSymbol(vm->mem, "pair");
  if (IsTuple(vm->mem, arg)) return MakeSymbol(vm->mem, "tuple");
  if (IsBinary(vm->mem, arg)) return MakeSymbol(vm->mem, "binary");
  return RuntimeError("Unknown type", arg, vm);
}

static Val IsNilOp(Val args, VM *vm)
{
  return BoolVal(IsNil(Head(vm->mem, args)));
}

static Val IsIntegerOp(Val args, VM *vm)
{
  return BoolVal(IsInt(Head(vm->mem, args)));
}

static Val IsFloatOp(Val args, VM *vm)
{
  return BoolVal(IsNum(Head(vm->mem, args)));
}

static Val IsNumericOp(Val args, VM *vm)
{
  return BoolVal(IsInt(Head(vm->mem, args)) || IsNum(Head(vm->mem, args)));
}

static Val IsSymbolOp(Val args, VM *vm)
{
  return BoolVal(IsSym(Head(vm->mem, args)));
}

static Val IsPairOp(Val args, VM *vm)
{
  return BoolVal(IsPair(Head(vm->mem, args)));
}

static Val IsTupleOp(Val args, VM *vm)
{
  return BoolVal(IsTuple(vm->mem, Head(vm->mem, args)));
}

static Val IsBinaryOp(Val args, VM *vm)
{
  return BoolVal(IsBinary(vm->mem, Head(vm->mem, args)));
}

static Val IsTrueOp(Val args, VM *vm)
{
  return BoolVal(IsTrue(Head(vm->mem, args)));
}

static Val NotOp(Val args, VM *vm)
{
  return BoolVal(!IsTrue(Head(vm->mem, args)));
}

static Val CeilOp(Val args, VM *vm)
{
  Val arg = Head(vm->mem, args);
  if (IsInt(arg)) return arg;
  if (IsNum(arg)) return NumVal(Ceil(RawNum(arg)));
  return RuntimeError("Bad argument to ceil", arg, vm);
}

static Val FloorOp(Val args, VM *vm)
{
  Val arg = Head(vm->mem, args);
  if (IsInt(arg)) return arg;
  if (IsNum(arg)) return NumVal(Floor(RawNum(arg)));
  return RuntimeError("Bad argument to floor", arg, vm);
}

static Val RoundOp(Val args, VM *vm)
{
  Val arg = Head(vm->mem, args);
  if (IsInt(arg)) {
    return arg;
  } else if (IsNum(arg)) {
    float n = RawNum(arg);
    float frac = n - Floor(n);
    if (frac == 0.5) {
      return IntVal(Floor(n) + (n > 0));
    }

    return IntVal(Floor(n) + (frac > 0.5));
  } else {
    return RuntimeError("Bad argument to round", arg, vm);
  }
}

static Val DivOp(Val args, VM *vm)
{
  Val a = First(vm->mem, args);
  Val b = Second(vm->mem, args);
  if (!IsInt(a)) return RuntimeError("Bad div argument", a, vm);
  if (!IsInt(b)) return RuntimeError("Bad div argument", b, vm);

  return IntVal(RawInt(a) / RawInt(b));
}

static Val RemOp(Val args, VM *vm)
{
  Val a = First(vm->mem, args);
  Val b = Second(vm->mem, args);
  if (!IsInt(a)) return RuntimeError("Bad rem argument", a, vm);
  if (!IsInt(b)) return RuntimeError("Bad rem argument", b, vm);

  i32 div = RawInt(a) / RawInt(b);
  return IntVal(RawInt(a) - RawInt(b)*div);
}

static Val AbsOp(Val args, VM *vm)
{
  Val arg = First(vm->mem, args);
  if (IsNum(arg)) return NumVal(Abs(RawNum(arg)));
  if (IsInt(arg)) return IntVal(Abs(RawInt(arg)));
  return RuntimeError("Bad argument to abs", arg, vm);
}

static Val MinOp(Val args, VM *vm)
{
  i32 min_int = MaxInt;
  float min_num = -Infinity;

  while (!IsNil(args)) {
    Val arg = Head(vm->mem, args);
    if (IsNum(arg)) {
      min_num = Min(min_num, RawNum(arg));
    } else if (IsInt(arg)) {
      min_int = Min(min_int, RawInt(arg));
    } else {
      return RuntimeError("Bad argument to min", arg, vm);
    }
  }

  if (min_num < (float)min_int) {
    return NumVal(min_num);
  } else {
    return IntVal(min_int);
  }
}

static Val MaxOp(Val args, VM *vm)
{
  i32 max_int = MinInt;
  float max_num = Infinity;

  while (!IsNil(args)) {
    Val arg = Head(vm->mem, args);
    if (IsNum(arg)) {
      max_num = Max(max_num, RawNum(arg));
    } else if (IsInt(arg)) {
      max_int = Max(max_int, RawInt(arg));
    } else {
      return RuntimeError("Bad argument to max", arg, vm);
    }
  }

  if (max_num > (float)max_int) {
    return NumVal(max_num);
  } else {
    return IntVal(max_int);
  }
}

static Val RandomOp(Val args, VM *vm)
{
  float r = (float)Random() / MaxUInt;
  return NumVal(r);
}

static Val RandomIntOp(Val args, VM *vm)
{
  Val a = First(vm->mem, args);
  Val b = Second(vm->mem, args);
  if (!IsInt(a)) return RuntimeError("Bad argument to random-int", a, vm);
  if (!IsInt(b)) return RuntimeError("Bad argument to random-int", b, vm);

  float r = (float)Random() / MaxUInt;
  i32 min = RawInt(a);
  i32 max = RawInt(b);
  return IntVal(Floor(r*(max - min) + min));
}

static Val PairOp(Val args, VM *vm)
{
  Val a = First(vm->mem, args);
  Val b = Second(vm->mem, args);
  return MakePair(vm->mem, a, b);
}

static Val HeadOp(Val args, VM *vm)
{
  Val arg = First(vm->mem, args);
  if (!IsPair(arg)) return RuntimeError("Bad argument to head", arg, vm);
  return Head(vm->mem, arg);
}

static Val TailOp(Val args, VM *vm)
{
  Val arg = First(vm->mem, args);
  if (!IsPair(arg)) return RuntimeError("Bad argument to tail", arg, vm);
  return Tail(vm->mem, First(vm->mem, args));
}

static Val SetHeadOp(Val args, VM *vm)
{
  Val pair = First(vm->mem, args);
  Val head = Second(vm->mem, args);
  if (!IsPair(pair)) return RuntimeError("Bad argument to set-head!", pair, vm);
  SetHead(vm->mem, pair, head);
  return SymbolFor("ok");
}

static Val SetTailOp(Val args, VM *vm)
{
  Val pair = First(vm->mem, args);
  Val tail = Second(vm->mem, args);
  if (!IsPair(pair)) return RuntimeError("Bad argument to set-tail!", pair, vm);
  SetTail(vm->mem, pair, tail);
  return SymbolFor("ok");
}

static Val TupleOp(Val args, VM *vm)
{
  u32 length = ListLength(vm->mem, args);
  Val tuple = MakeTuple(vm->mem, length);
  for (u32 i = 0; i < length; i++) {
    TupleSet(vm->mem, tuple, i, Head(vm->mem, args));
    args = Tail(vm->mem, args);
  }
  return tuple;
}

static Val TupleSizeOp(Val args, VM *vm)
{
  Val arg = First(vm->mem, args);
  if (!IsTuple(vm->mem, arg)) return RuntimeError("Bad argument to tuple", arg, vm);
  return IntVal(TupleLength(vm->mem, arg));
}

static Val GetNthOp(Val args, VM *vm)
{
  Val arg = First(vm->mem, args);
  Val n = Second(vm->mem, args);
  if (!IsTuple(vm->mem, arg)) return RuntimeError("Bad argument to tuple", arg, vm);
  if (!IsInt(n)) return RuntimeError("Bad argument to tuple", n, vm);
  return TupleAt(vm->mem, arg, RawInt(n));
}

static Val SetNthOp(Val args, VM *vm)
{
  Val arg = First(vm->mem, args);
  Val n = Second(vm->mem, args);
  Val value = Third(vm->mem, args);
  if (!IsTuple(vm->mem, arg)) return RuntimeError("Bad argument to tuple", arg, vm);
  if (!IsInt(n)) return RuntimeError("Bad argument to tuple", n, vm);
  TupleSet(vm->mem, arg, RawInt(n), value);
  return SymbolFor("ok");
}

static Val MapOp(Val args, VM *vm)
{
  return RuntimeError("Maps not yet implemented", nil, vm);
}

static Val MapSizeOp(Val args, VM *vm)
{
  return RuntimeError("Maps not yet implemented", nil, vm);
}

static Val MapGetOp(Val args, VM *vm)
{
  return RuntimeError("Maps not yet implemented", nil, vm);
}

static Val MapSetOp(Val args, VM *vm)
{
  return RuntimeError("Maps not yet implemented", nil, vm);
}

static Val AllocateOp(Val args, VM *vm)
{
  Val size = First(vm->mem, args);
  if (!IsInt(size)) return RuntimeError("Bad argument to allocate", size, vm);
  return MakeBinary(vm->mem, RawInt(size));
}

static Val ByteSizeOp(Val args, VM *vm)
{
  Val binary = First(vm->mem, args);
  if (!IsBinary(vm->mem, binary)) return RuntimeError("Bad argument to byte-size", binary, vm);
  return IntVal(BinaryLength(vm->mem, binary));
}

static Val GetByteOp(Val args, VM *vm)
{
  Val binary = First(vm->mem, args);
  Val n = Second(vm->mem, args);
  if (!IsBinary(vm->mem, binary)) return RuntimeError("Bad argument to get-byte", binary, vm);
  if (!IsInt(n)) return RuntimeError("Bad argument to get-byte", n, vm);
  u8 *data = BinaryData(vm->mem, binary);
  return IntVal(data[RawInt(n)]);
}

static Val SetByteOp(Val args, VM *vm)
{
  Val binary = First(vm->mem, args);
  Val n = Second(vm->mem, args);
  Val byte = Third(vm->mem, args);
  if (!IsBinary(vm->mem, binary)) return RuntimeError("Bad argument to set-byte!", binary, vm);
  if (!IsInt(n)) return RuntimeError("Bad argument to set-byte!", n, vm);
  if (!IsInt(byte) || RawInt(byte) < 0 || RawInt(byte) > 255) return RuntimeError("Bad argument to set-byte!", byte, vm);
  u8 *data = BinaryData(vm->mem, binary);
  data[RawInt(n)] = (u8)RawInt(byte);
  return SymbolFor("ok");
}

static Val ToStringOp(Val args, VM *vm)
{
  Val arg = First(vm->mem, args);
  char str[255];

  if (IsBinary(vm->mem, arg)) {
    return arg;
  } else if (IsSym(arg)) {
    char *name = SymbolName(vm->mem, arg);
    return MakeBinaryFrom(vm->mem, name, SymbolLength(vm->mem, arg));
  } else if (IsNum(arg)) {
    u32 length = FloatToString(RawNum(arg), str, 255);
    return MakeBinaryFrom(vm->mem, str, length);
  } else if (IsInt(arg)) {
    u32 length = IntToString(RawInt(arg), str, 255);
    return MakeBinaryFrom(vm->mem, str, length);
  } else {
    return RuntimeError("Not string representable", arg, vm);
  }
}

static Val PrintOp(Val args, VM *vm)
{
  while (!IsNil(args)) {
    Val message = Head(vm->mem, args);
    if (ValToString(vm->mem, message, output) == 0) {
      return RuntimeError("Could not print", message, vm);
    }
    args = Tail(vm->mem, args);
  }
  Print("\n");

  return SymbolFor("ok");
}

static Val EvalOp(Val args, VM *vm)
{
  return RuntimeError("Eval not implemented", nil, vm);
}

static Val ApplyOp(Val args, VM *vm)
{
  return RuntimeError("Apply not implemented", nil, vm);
}


