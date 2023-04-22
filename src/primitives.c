#include "primitives.h"
#include "env.h"
#include "interpret.h"
#include "value.h"

typedef Val (*PrimitiveFn)(Val op, Val args, VM *vm);

typedef struct {
  char *name;
  PrimitiveFn fn;
} Primitive;

#define NumOp(a, op, b)   RawNum(a) op RawNum(b)
#define IntOp(a, op, b)   RawInt(a) op RawInt(b)

Val NumberOp(Val op, Val args, VM *vm)
{
  Val a = ListAt(vm->mem, args, 0);
  Val b = ListAt(vm->mem, args, 1);

  if (!IsNumeric(a)) {
    return RuntimeError("Bad arithmetic argument", a, vm->mem);
  }
  if (!IsNumeric(b)) {
    return RuntimeError("Bad arithmetic argument", b, vm->mem);
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

  return RuntimeError("Unimplemented primitive", op, vm->mem);
}

Val LogicOp(Val op, Val args, VM *vm)
{
  Val a = ListAt(vm->mem, args, 0);
  Val b = ListAt(vm->mem, args, 1);

  if (Eq(op, SymbolFor("=="))) return BoolVal(Eq(a, b));
  if (Eq(op, SymbolFor("!="))) return BoolVal(!Eq(a, b));
  if (Eq(op, SymbolFor("and"))) return BoolVal(IsTrue(a) && IsTrue(b));
  if (Eq(op, SymbolFor("or"))) return BoolVal(IsTrue(a) || IsTrue(b));

  return RuntimeError("Unimplemented primitive", op, vm->mem);
}

Val PairOp(Val op, Val args, VM *vm)
{
  Val a = ListAt(vm->mem, args, 0);
  Val b = ListAt(vm->mem, args, 1);
  return MakePair(vm->mem, a, b);
}

Val ConcatOp(Val op, Val args, VM *vm)
{
  Val a = ListAt(vm->mem, args, 0);
  Val b = ListAt(vm->mem, args, 1);
  return ListConcat(vm->mem, a, b);
}

Val DictMergeOp(Val op, Val args, VM *vm)
{
  Val dict = ListAt(vm->mem, args, 0);
  Val updates = ListAt(vm->mem, args, 1);
  Val keys = DictKeys(vm->mem, updates);
  Val vals = DictValues(vm->mem, updates);
  for (u32 i = 0; i < DictSize(vm->mem, updates); i++) {
    dict = DictSet(vm->mem, dict, TupleAt(vm->mem, keys, i), TupleAt(vm->mem, vals, i));
  }
  return dict;
}

Val HeadOp(Val op, Val args, VM *vm)
{
  return Head(vm->mem, Head(vm->mem, args));
}

Val TailOp(Val op, Val args, VM *vm)
{
  return Tail(vm->mem, Head(vm->mem, args));
}

Val ListOp(Val op, Val args, VM *vm)
{
  return args;
}

Val TupleOp(Val op, Val args, VM *vm)
{
  u32 length = ListLength(vm->mem, args);
  Val tuple = MakeTuple(vm->mem, length);
  for (u32 i = 0; i < length; i++) {
    TupleSet(vm->mem, tuple, i, Head(vm->mem, args));
    args = Tail(vm->mem, args);
  }
  return tuple;
}

Val DictOp(Val op, Val args, VM *vm)
{
  u32 length = ListLength(vm->mem, args) / 2;
  Val keys = MakeTuple(vm->mem, length);
  Val vals = MakeTuple(vm->mem, length);
  u32 i = 0;
  while (!IsNil(args)) {
    TupleSet(vm->mem, keys, i, Head(vm->mem, args));
    args = Tail(vm->mem, args);
    TupleSet(vm->mem, vals, i, Head(vm->mem, args));
    args = Tail(vm->mem, args);
    i++;
  }
  Val dict = DictFrom(vm->mem, keys, vals);
  return dict;
}

Val PrintOp(Val op, Val args, VM *vm)
{
  while (!IsNil(args)) {
    PrintVal(vm->mem, Head(vm->mem, args));
    args = Tail(vm->mem, args);
    if (!IsNil(args)) Print(" ");
  }
  Print("\n");
  return nil;
}

Val TypeOp(Val op, Val args, VM *vm)
{
  Val arg = Head(vm->mem, args);

  if (Eq(op, SymbolFor("nil?"))) return BoolVal(IsNil(arg));
  if (Eq(op, SymbolFor("integer?"))) return BoolVal(IsInt(arg));
  if (Eq(op, SymbolFor("float?"))) return BoolVal(IsNum(arg));
  if (Eq(op, SymbolFor("number?"))) return BoolVal(IsNumeric(arg));
  if (Eq(op, SymbolFor("symbol?"))) return BoolVal(IsSym(arg));
  if (Eq(op, SymbolFor("list?"))) return BoolVal(IsList(vm->mem, arg));
  if (Eq(op, SymbolFor("tuple?"))) return BoolVal(IsTuple(vm->mem, arg));
  if (Eq(op, SymbolFor("dict?"))) return BoolVal(IsDict(vm->mem, arg));
  if (Eq(op, SymbolFor("binary?"))) return BoolVal(IsBinary(vm->mem, arg));

  return RuntimeError("Unimplemented primitive", op, vm->mem);
}

Val AbsOp(Val op, Val args, VM *vm)
{
  Val arg = Head(vm->mem, args);
  if (IsNum(arg)) return NumVal(Abs(RawNum(arg)));
  if (IsInt(arg)) return IntVal(Abs(RawInt(arg)));
  return RuntimeError("Bad argument to abs", arg, vm->mem);
}

Val ApplyOp(Val op, Val args, VM *vm)
{
  Val proc = ListAt(vm->mem, args, 0);
  args = ListAt(vm->mem, args, 1);
  return Apply(proc, args, vm);
}

Val CeilOp(Val op, Val args, VM *vm)
{
  Val arg = Head(vm->mem, args);
  if (IsInt(arg)) return arg;
  if (IsNum(arg)) return NumVal(Ceil(RawNum(arg)));
  return RuntimeError("Bad argument to ceil", arg, vm->mem);
}

Val FloorOp(Val op, Val args, VM *vm)
{
  Val arg = Head(vm->mem, args);
  if (IsInt(arg)) return arg;
  if (IsNum(arg)) return NumVal(Floor(RawNum(arg)));
  return RuntimeError("Bad argument to floor", arg, vm->mem);
}

Val DivOp(Val op, Val args, VM *vm)
{
  Val a = ListAt(vm->mem, args, 0);
  Val b = ListAt(vm->mem, args, 1);
  if (!IsInt(a)) return RuntimeError("Bad argument to rem", a, vm->mem);
  if (!IsInt(b)) return RuntimeError("Bad argument to rem", b, vm->mem);
  return IntVal(RawInt(a) / RawInt(b));
}

Val RemOp(Val op, Val args, VM *vm)
{
  Val a = ListAt(vm->mem, args, 0);
  Val b = ListAt(vm->mem, args, 1);
  if (!IsInt(a)) return RuntimeError("Bad argument to rem", a, vm->mem);
  if (!IsInt(b)) return RuntimeError("Bad argument to rem", b, vm->mem);
  return IntVal(RawInt(a) % RawInt(b));
}

Val LengthOp(Val op, Val args, VM *vm)
{
  Val arg = Head(vm->mem, args);
  if (IsList(vm->mem, arg)) return IntVal(ListLength(vm->mem, arg));
  if (IsTuple(vm->mem, arg)) return IntVal(TupleLength(vm->mem, arg));
  if (IsDict(vm->mem, arg)) return IntVal(DictSize(vm->mem, arg));
  if (IsBinary(vm->mem, arg)) return IntVal(BinaryLength(vm->mem, arg));
  return RuntimeError("Cannot take length of this", arg, vm->mem);
}

Val MaxOp(Val op, Val args, VM *vm)
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
      return RuntimeError("Bad argument to max", arg, vm->mem);
    }
  }

  if (max_num > (float)max_int) {
    return NumVal(max_num);
  } else {
    return IntVal(max_int);
  }
}

Val MinOp(Val op, Val args, VM *vm)
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
      return RuntimeError("Bad argument to min", arg, vm->mem);
    }
  }

  if (min_num < (float)min_int) {
    return NumVal(min_num);
  } else {
    return IntVal(min_int);
  }
}

Val NotOp(Val op, Val args, VM *vm)
{
  Val arg = Head(vm->mem, args);
  return BoolVal(!IsTrue(arg));
}

Val RoundOp(Val op, Val args, VM *vm)
{
  Val arg = Head(vm->mem, args);
  if (IsInt(arg)) {
    return arg;
  } else if (IsNum(arg)) {
    // return NumVal()
    return arg;
  } else {
    return RuntimeError("Bad argument to round", arg, vm->mem);
  }
}

Val ToStringOp(Val op, Val args, VM *vm)
{
  Val arg = Head(vm->mem, args);
  char str[255];

  if (IsBinary(vm->mem, arg)) return arg;
  if (IsSym(arg)) return MakeBinary(vm->mem, SymbolName(vm->mem, arg));
  if (IsNum(arg)) {
    u32 length = FloatToString(RawNum(arg), str, 255);
    return MakeBinaryFrom(vm->mem, str, length);
  }
  if (IsInt(arg)) {
    u32 length = IntToString(RawInt(arg), str, 255);
    return MakeBinaryFrom(vm->mem, str, length);
  }

  return RuntimeError("Not string representable", arg, vm->mem);
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
  {"pair", PairOp},
  {"[+", ConcatOp},
  {"{|", DictMergeOp},
  {"[", ListOp},
  {"#[", TupleOp},
  {"{", DictOp},
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

Val DoPrimitive(Val op, Val args, VM *vm)
{
  for (u32 i = 0; i < ArrayCount(primitives); i++) {
    if (Eq(SymbolFor(primitives[i].name), op)) {
      return primitives[i].fn(op, args, vm);
    }
  }

  return RuntimeError("Unknown primitive", op, vm->mem);
}

void DefinePrimitives(Val env, Mem *mem)
{
  Val prim = MakeSymbol(mem, "α");
  for (u32 i = 0; i < ArrayCount(primitives); i++) {
    Val sym = MakeSymbol(mem, primitives[i].name);
    Define(sym, MakePair(mem, prim, sym), env, mem);
  }
}
