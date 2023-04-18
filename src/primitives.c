#include "primitives.h"
#include "env.h"
#include "interpret.h"
#include "mem.h"
#include "value.h"

typedef Val (*PrimitiveFn)(Val op, Val args, Mem *mem);

typedef struct {
  char *name;
  PrimitiveFn fn;
} Primitive;

#define NumOp(a, op, b)   RawNum(a) op RawNum(b)
#define IntOp(a, op, b)   RawInt(a) op RawInt(b)

Val NumberOp(Val op, Val args, Mem *mem)
{
  Val a = ListAt(mem, args, 0);
  Val b = ListAt(mem, args, 1);

  if (!IsNumeric(a)) {
    return RuntimeError("Bad arithmetic argument", a, mem);
  }
  if (!IsNumeric(b)) {
    return RuntimeError("Bad arithmetic argument", b, mem);
  }

  if (IsInt(a) && IsInt(b)) {
    if (Eq(op, SymbolFor("+"))) return IntVal(IntOp(a, +, b));
    if (Eq(op, SymbolFor("-"))) return IntVal(IntOp(a, -, b));
    if (Eq(op, SymbolFor("*"))) return IntVal(IntOp(a, *, b));
  }

  if (Eq(op, SymbolFor("+"))) return NumVal(NumOp(a, +, b));
  if (Eq(op, SymbolFor("-"))) return NumVal(NumOp(a, -, b));
  if (Eq(op, SymbolFor("*"))) return NumVal(NumOp(a, *, b));
  if (Eq(op, SymbolFor("/"))) return NumVal(NumOp(a, /, b));
  if (Eq(op, SymbolFor(">"))) return BoolVal(NumOp(a, >, b));
  if (Eq(op, SymbolFor("<"))) return BoolVal(NumOp(a, <, b));
  if (Eq(op, SymbolFor(">="))) return BoolVal(NumOp(a, >=, b));
  if (Eq(op, SymbolFor("<="))) return BoolVal(NumOp(a, <=, b));

  return RuntimeError("Unimplemented primitive", op, mem);
}

Val LogicOp(Val op, Val args, Mem *mem)
{
  Val a = ListAt(mem, args, 0);
  Val b = ListAt(mem, args, 1);

  if (Eq(op, SymbolFor("=="))) return BoolVal(Eq(a, b));
  if (Eq(op, SymbolFor("!="))) return BoolVal(!Eq(a, b));
  if (Eq(op, SymbolFor("and"))) return BoolVal(IsTrue(a) && IsTrue(b));
  if (Eq(op, SymbolFor("or"))) return BoolVal(IsTrue(a) || IsTrue(b));

  return RuntimeError("Unimplemented primitive", op, mem);
}

Val PairOp(Val op, Val args, Mem *mem)
{
  Val a = ListAt(mem, args, 0);
  Val b = ListAt(mem, args, 1);
  return MakePair(mem, a, b);
}

Val ConcatOp(Val op, Val args, Mem *mem)
{
  Val a = ListAt(mem, args, 0);
  Val b = ListAt(mem, args, 1);
  return ListConcat(mem, a, b);
}

Val DictMergeOp(Val op, Val args, Mem *mem)
{
  Val dict = ListAt(mem, args, 0);
  Val updates = ListAt(mem, args, 1);
  Val keys = DictKeys(mem, updates);
  Val vals = DictValues(mem, updates);
  for (u32 i = 0; i < DictSize(mem, updates); i++) {
    dict = DictSet(mem, dict, TupleAt(mem, keys, i), TupleAt(mem, vals, i));
  }
  return dict;
}

Val HeadOp(Val op, Val args, Mem *mem)
{
  return Head(mem, Head(mem, args));
}

Val TailOp(Val op, Val args, Mem *mem)
{
  return Tail(mem, Head(mem, args));
}

Val ListOp(Val op, Val args, Mem *mem)
{
  return args;
}

Val TupleOp(Val op, Val args, Mem *mem)
{
  u32 length = ListLength(mem, args);
  Val tuple = MakeTuple(mem, length);
  for (u32 i = 0; i < length; i++) {
    TupleSet(mem, tuple, i, Head(mem, args));
    args = Tail(mem, args);
  }
  return tuple;
}

Val DictOp(Val op, Val args, Mem *mem)
{
  u32 length = ListLength(mem, args) / 2;
  Val keys = MakeTuple(mem, length);
  Val vals = MakeTuple(mem, length);
  u32 i = 0;
  while (!IsNil(args)) {
    TupleSet(mem, keys, i, Head(mem, args));
    args = Tail(mem, args);
    TupleSet(mem, vals, i, Head(mem, args));
    args = Tail(mem, args);
    i++;
  }
  Val dict = DictFrom(mem, keys, vals);
  return dict;
}

Val PrintOp(Val op, Val args, Mem *mem)
{
  while (!IsNil(args)) {
    PrintVal(mem, Head(mem, args));
    args = Tail(mem, args);
    if (!IsNil(args)) Print(" ");
  }
  Print("\n");
  return nil;
}

Val TypeOp(Val op, Val args, Mem *mem)
{
  Val arg = Head(mem, args);

  if (Eq(op, SymbolFor("nil?"))) return BoolVal(IsNil(arg));
  if (Eq(op, SymbolFor("integer?"))) return BoolVal(IsInt(arg));
  if (Eq(op, SymbolFor("float?"))) return BoolVal(IsNum(arg));
  if (Eq(op, SymbolFor("number?"))) return BoolVal(IsNumeric(arg));
  if (Eq(op, SymbolFor("symbol?"))) return BoolVal(IsSym(arg));
  if (Eq(op, SymbolFor("list?"))) return BoolVal(IsList(mem, arg));
  if (Eq(op, SymbolFor("tuple?"))) return BoolVal(IsTuple(mem, arg));
  if (Eq(op, SymbolFor("dict?"))) return BoolVal(IsDict(mem, arg));
  if (Eq(op, SymbolFor("binary?"))) return BoolVal(IsBinary(mem, arg));

  return RuntimeError("Unimplemented primitive", op, mem);
}

Val AbsOp(Val op, Val args, Mem *mem)
{
  Val arg = Head(mem, args);
  if (IsNum(arg)) return NumVal(Abs(RawNum(arg)));
  if (IsInt(arg)) return IntVal(Abs(RawInt(arg)));
  return RuntimeError("Bad argument to abs", arg, mem);
}

Val ApplyOp(Val op, Val args, Mem *mem)
{
  Val proc = ListAt(mem, args, 0);
  args = ListAt(mem, args, 1);
  return Apply(proc, args, mem);
}

Val CeilOp(Val op, Val args, Mem *mem)
{
  Val arg = Head(mem, args);
  if (IsInt(arg)) return arg;
  // if (IsNum(arg)) return NumVal(Ceil(RawNum(arg)));
  return RuntimeError("Bad argument to ceil", arg, mem);
}

Val FloorOp(Val op, Val args, Mem *mem)
{
  Val arg = Head(mem, args);
  if (IsInt(arg)) return arg;
  // if (IsNum(arg)) return NumVal(Floor(RawNum(arg)));
  return RuntimeError("Bad argument to floor", arg, mem);
}

Val DivOp(Val op, Val args, Mem *mem)
{
  Val a = ListAt(mem, args, 0);
  Val b = ListAt(mem, args, 1);
  if (!IsInt(a)) return RuntimeError("Bad argument to rem", a, mem);
  if (!IsInt(b)) return RuntimeError("Bad argument to rem", b, mem);
  return IntVal(RawInt(a) / RawInt(b));
}

Val RemOp(Val op, Val args, Mem *mem)
{
  Val a = ListAt(mem, args, 0);
  Val b = ListAt(mem, args, 1);
  if (!IsInt(a)) return RuntimeError("Bad argument to rem", a, mem);
  if (!IsInt(b)) return RuntimeError("Bad argument to rem", b, mem);
  return IntVal(RawInt(a) % RawInt(b));
}

Val LengthOp(Val op, Val args, Mem *mem)
{
  Val arg = Head(mem, args);
  if (IsList(mem, arg)) return IntVal(ListLength(mem, arg));
  if (IsTuple(mem, arg)) return IntVal(TupleLength(mem, arg));
  if (IsDict(mem, arg)) return IntVal(DictSize(mem, arg));
  if (IsBinary(mem, arg)) return IntVal(BinaryLength(mem, arg));
  return RuntimeError("Cannot take length of this", arg, mem);
}

Val MaxOp(Val op, Val args, Mem *mem)
{
  i32 max_int = MinInt;
  float max_num = Infinity;

  while (!IsNil(args)) {
    Val arg = Head(mem, args);
    if (IsNum(arg)) {
      max_num = Max(max_num, RawNum(arg));
    } else if (IsInt(arg)) {
      max_int = Max(max_int, RawInt(arg));
    } else {
      return RuntimeError("Bad argument to max", arg, mem);
    }
  }

  if (max_num > (float)max_int) {
    return NumVal(max_num);
  } else {
    return IntVal(max_int);
  }
}

Val MinOp(Val op, Val args, Mem *mem)
{
  i32 min_int = MaxInt;
  float min_num = -Infinity;

  while (!IsNil(args)) {
    Val arg = Head(mem, args);
    if (IsNum(arg)) {
      min_num = Min(min_num, RawNum(arg));
    } else if (IsInt(arg)) {
      min_int = Min(min_int, RawInt(arg));
    } else {
      return RuntimeError("Bad argument to min", arg, mem);
    }
  }

  if (min_num < (float)min_int) {
    return NumVal(min_num);
  } else {
    return IntVal(min_int);
  }
}

Val NotOp(Val op, Val args, Mem *mem)
{
  Val arg = Head(mem, args);
  return BoolVal(!IsTrue(arg));
}

Val RoundOp(Val op, Val args, Mem *mem)
{
  Val arg = Head(mem, args);
  if (IsInt(arg)) {
    return arg;
  } else if (IsNum(arg)) {
    // return NumVal()
    return arg;
  } else {
    return RuntimeError("Bad argument to round", arg, mem);
  }
}

Val ToStringOp(Val op, Val args, Mem *mem)
{
  Val arg = Head(mem, args);
  char str[255];

  if (IsBinary(mem, arg)) return arg;
  if (IsSym(arg)) return MakeBinary(mem, SymbolName(mem, arg));
  if (IsNum(arg)) {
    u32 length = FloatToString(RawNum(arg), str, 255);
    return MakeBinaryFrom(mem, str, length);
  }
  if (IsInt(arg)) {
    u32 length = IntToString(RawInt(arg), str, 255);
    return MakeBinaryFrom(mem, str, length);
  }

  return RuntimeError("Not string representable", arg, mem);
}

static Primitive primitives[] = {
  {"*", NumberOp},
  {"/", NumberOp},
  {"+", NumberOp},
  {"-", NumberOp},
  {">", NumberOp},
  {"<", NumberOp},
  {">=", NumberOp},
  {"<=", NumberOp},
  {"and", LogicOp},
  {"or", LogicOp},
  {"==", LogicOp},
  {"!=", LogicOp},
  {"__pair", PairOp},
  {"__concat", ConcatOp},
  {"__dict-merge", DictMergeOp},
  {"__list", ListOp},
  {"__tuple", TupleOp},
  {"__dict", DictOp},
  {"head", HeadOp},
  {"tail", TailOp},
  {"rem", RemOp},
  {"nil?", TypeOp},
  {"integer?", TypeOp},
  {"float?", TypeOp},
  {"number?", TypeOp},
  {"symbol?", TypeOp},
  {"list?", TypeOp},
  {"tuple?", TypeOp},
  {"dict?", TypeOp},
  {"binary?", TypeOp},
  {"print", PrintOp},
  {"abs", AbsOp},
  {"apply", ApplyOp},
  {"ceil", CeilOp},
  {"floor", FloorOp},
  {"div", DivOp},
  {"rem", RemOp},
  {"length", LengthOp},
  {"max", MaxOp},
  {"min", MinOp},
  {"not", NotOp},
  {"round", RoundOp},
  {"to-string", ToStringOp},
};

Val DoPrimitive(Val op, Val args, Mem *mem)
{
  for (u32 i = 0; i < ArrayCount(primitives); i++) {
    if (Eq(SymbolFor(primitives[i].name), op)) {
      return primitives[i].fn(op, args, mem);
    }
  }

  return RuntimeError("Unknown primitive", op, mem);
}

void DefinePrimitives(Val env, Mem *mem)
{
  Val prim = MakeSymbol(mem, "__primitive");
  for (u32 i = 0; i < ArrayCount(primitives); i++) {
    Val sym = MakeSymbol(mem, primitives[i].name);
    Define(sym, MakePair(mem, prim, sym), env, mem);
  }
}

/*
abs
apply
ceil
floor
div
rem
exit
length
max
min
not
round
to-string
*/
