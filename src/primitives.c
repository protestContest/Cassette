#include "primitives.h"
#include "env.h"
#include "value.h"
#include "port.h"

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
    return RuntimeError("Bad arithmetic argument", a, vm);
  }
  if (!IsNumeric(b)) {
    return RuntimeError("Bad arithmetic argument", b, vm);
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

  return RuntimeError("Unimplemented primitive", op, vm);
}

Val LogicOp(Val op, Val args, VM *vm)
{
  Val a = ListAt(vm->mem, args, 0);
  Val b = ListAt(vm->mem, args, 1);

  if (Eq(op, SymbolFor("=="))) return BoolVal(Eq(a, b));
  if (Eq(op, SymbolFor("!="))) return BoolVal(!Eq(a, b));

  return RuntimeError("Unimplemented primitive", op, vm);
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

Val NthOp(Val op, Val args, VM *vm)
{
  Val list = ListAt(vm->mem, args, 0);
  Val n = ListAt(vm->mem, args, 1);
  return ListAt(vm->mem, list, RawInt(n));
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

Val AccessOp(Val op, Val args, VM *vm)
{
  Val dict = ListAt(vm->mem, args, 0);
  Val key = ListAt(vm->mem, args, 1);
  return DictGet(vm->mem, dict, key);
}

Val StringOp(Val op, Val args, VM *vm)
{
  Val symbol = Head(vm->mem, args);
  return MakeBinary(vm->mem, SymbolName(vm->mem, symbol));
}

Val PrintOp(Val op, Val args, VM *vm)
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
  if (Eq(op, SymbolFor("true?"))) return BoolVal(IsTrue(arg));

  return RuntimeError("Unimplemented primitive", op, vm);
}

Val AbsOp(Val op, Val args, VM *vm)
{
  Val arg = Head(vm->mem, args);
  if (IsNum(arg)) return NumVal(Abs(RawNum(arg)));
  if (IsInt(arg)) return IntVal(Abs(RawInt(arg)));
  return RuntimeError("Bad argument to abs", arg, vm);
}

// Val ApplyOp(Val op, Val args, VM *vm)
// {
//   Val proc = ListAt(vm->mem, args, 0);
//   args = ListAt(vm->mem, args, 1);
//   return Apply(proc, args, vm);
// }

// Val EvalOp(Val op, Val args, VM *vm)
// {
//   Val exp = ListAt(vm->mem, args, 0);

//   if (IsBinary(vm->mem, exp)) {
//     Source src = {"eval", (char*)BinaryData(vm->mem, exp), BinaryLength(vm->mem, exp)};
//     exp = Parse(src, vm->mem);
//   }

//   return Eval(exp, vm);
// }

Val CeilOp(Val op, Val args, VM *vm)
{
  Val arg = Head(vm->mem, args);
  if (IsInt(arg)) return arg;
  if (IsNum(arg)) return NumVal(Ceil(RawNum(arg)));
  return RuntimeError("Bad argument to ceil", arg, vm);
}

Val FloorOp(Val op, Val args, VM *vm)
{
  Val arg = Head(vm->mem, args);
  if (IsInt(arg)) return arg;
  if (IsNum(arg)) return NumVal(Floor(RawNum(arg)));
  return RuntimeError("Bad argument to floor", arg, vm);
}

Val DivOp(Val op, Val args, VM *vm)
{
  Val a = ListAt(vm->mem, args, 0);
  Val b = ListAt(vm->mem, args, 1);
  if (!IsInt(a)) {
    return RuntimeError("Bad division argument", a, vm);
  }
  if (!IsInt(b)) {
    return RuntimeError("Bad division argument", b, vm);
  }

  i32 ra = RawInt(a);
  i32 rb = RawInt(b);

  if (Eq(op, SymbolFor("div"))) {
    return IntVal(ra / rb);
  }

  if (Eq(op, SymbolFor("rem"))) {
    i32 div = ra / rb;
    return IntVal(ra - rb*div);
  }

  if (Eq(op, SymbolFor("mod"))) {
    i32 div = ra / rb;
    if ((ra < 0 && rb >= 0) || (ra >= 0 && rb < 0)) {
      --div;
    }
    return IntVal(ra - rb*div);
  }

  return RuntimeError("Unimplemented primitive", op, vm);
}

Val LengthOp(Val op, Val args, VM *vm)
{
  Val arg = Head(vm->mem, args);
  if (IsList(vm->mem, arg)) return IntVal(ListLength(vm->mem, arg));
  if (IsTuple(vm->mem, arg)) return IntVal(TupleLength(vm->mem, arg));
  if (IsDict(vm->mem, arg)) return IntVal(DictSize(vm->mem, arg));
  if (IsBinary(vm->mem, arg)) return IntVal(BinaryLength(vm->mem, arg));
  return RuntimeError("Cannot take length of this", arg, vm);
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
      return RuntimeError("Bad argument to max", arg, vm);
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
      return RuntimeError("Bad argument to min", arg, vm);
    }
  }

  if (min_num < (float)min_int) {
    return NumVal(min_num);
  } else {
    return IntVal(min_int);
  }
}

Val RandOp(Val op, Val args, VM *vm)
{
  float r = (float)Random() / MaxUInt;

  if (Eq(op, SymbolFor("random"))) {
    return NumVal(r);
  }

  if (Eq(op, SymbolFor("random-int"))) {
    i32 min = RawInt(ListAt(vm->mem, args, 0));
    i32 max = RawInt(ListAt(vm->mem, args, 1));
    return IntVal(Floor(r*(max - min) + min));
  }

  return RuntimeError("Unimplemented primitive", op, vm);
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

  return RuntimeError("Not string representable", arg, vm);
}

Val OpenOp(Val op, Val args, VM *vm)
{
  return RuntimeError("IO unimplemented", op, vm);
//   Val type = ListAt(vm->mem, args, 0);
//   Val name = ListAt(vm->mem, args, 1);
//   Val opts = ListAt(vm->mem, args, 2);

//   return OpenPort(vm, type, name, opts);
}

Val SendOp(Val op, Val args, VM *vm)
{
  return RuntimeError("IO unimplemented", op, vm);
//   Val port = ListAt(vm->mem, args, 0);
//   Val message = ListAt(vm->mem, args, 1);
//   return SendPort(vm, port, message);
}

Val RecvOp(Val op, Val args, VM *vm)
{
  return RuntimeError("IO unimplemented", op, vm);
//   Val port = ListAt(vm->mem, args, 0);
//   return RecvPort(vm, port);
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
  {"==", LogicOp},
  {"!=", LogicOp},
  {"|", PairOp},
  {"[+", ConcatOp},
  {"{|", DictMergeOp},
  {"[", ListOp},
  {"#[", TupleOp},
  {"{", DictOp},
  {".", AccessOp},
  {"\"", StringOp},
  {"head", HeadOp},
  {"tail", TailOp},
  {"nth", NthOp},
  {"length", LengthOp},
  {"nil?", TypeOp},
  {"integer?", TypeOp},
  {"float?", TypeOp},
  {"number?", TypeOp},
  {"symbol?", TypeOp},
  {"list?", TypeOp},
  {"tuple?", TypeOp},
  {"dict?", TypeOp},
  {"binary?", TypeOp},
  {"true?", TypeOp},
  {"not", NotOp},
  {"to-string", ToStringOp},
  {"abs", AbsOp},
  {"ceil", CeilOp},
  {"floor", FloorOp},
  {"div", DivOp},
  {"mod", DivOp},
  {"rem", DivOp},
  {"max", MaxOp},
  {"min", MinOp},
  {"round", RoundOp},
  {"random", RandOp},
  {"random-int", RandOp},
  {"open", OpenOp},
  {"send", SendOp},
  {"recv", RecvOp},
  {"print", PrintOp},
  // {"apply", ApplyOp},
  // {"eval", EvalOp},
};

bool IsPrimitive(Val op, Mem *mem)
{
  return IsTagged(mem, op, "α");
}

Val DoPrimitive(Val op, Val args, VM *vm)
{
  Val name = Tail(vm->mem, op);
  for (u32 i = 0; i < ArrayCount(primitives); i++) {
    if (Eq(SymbolFor(primitives[i].name), name)) {
      return primitives[i].fn(name, args, vm);
    }
  }

  return RuntimeError("Unknown primitive", op, vm);
}

void DefinePrimitives(Val env, Mem *mem)
{
  Val prim = MakeSymbol(mem, "α");
  for (u32 i = 0; i < ArrayCount(primitives); i++) {
    Val sym = MakeSymbol(mem, primitives[i].name);
    Define(sym, MakePair(mem, prim, sym), env, mem);
  }
}
