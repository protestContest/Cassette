#include "primitives.h"
#include "env.h"
#include "interpret.h"
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

  return RuntimeError("Unimplemented primitive", op, mem);
}

Val LogicOp(Val op, Val args, Mem *mem)
{
  Val a = ListAt(mem, args, 0);
  Val b = ListAt(mem, args, 1);

  if (Eq(op, SymbolFor("=="))) return BoolVal(Eq(a, b));
  if (Eq(op, SymbolFor("and"))) return BoolVal(IsTrue(a) && IsTrue(b));
  if (Eq(op, SymbolFor("or"))) return BoolVal(IsTrue(a) || IsTrue(b));

  return RuntimeError("Unimplemented primitive", op, mem);
}

Val PairOp(Val op, Val args, Mem *mem)
{
  return MakePair(mem, ListAt(mem, args, 0), ListAt(mem, args, 1));
}

Val HeadOp(Val op, Val args, Mem *mem)
{
  return Head(mem, Head(mem, args));
}

Val TailOp(Val op, Val args, Mem *mem)
{
  return Tail(mem, Head(mem, args));
}

Val RemOp(Val op, Val args, Mem *mem)
{
  Val a = ListAt(mem, args, 0);
  Val b = ListAt(mem, args, 1);
  if (!IsInt(a)) return RuntimeError("Bad argument to rem", a, mem);
  if (!IsInt(b)) return RuntimeError("Bad argument to rem", b, mem);
  return IntVal(RawInt(a) % RawInt(b));
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
  }
  return nil;
}

static Primitive primitives[] = {
  {"*", NumberOp},
  {"/", NumberOp},
  {"+", NumberOp},
  {"-", NumberOp},
  {">", NumberOp},
  {"<", NumberOp},
  {"and", LogicOp},
  {"or", LogicOp},
  {"==", LogicOp},
  {"|", PairOp},
  {"list", ListOp},
  {"tuple", TupleOp},
  {"dict", DictOp},
  {"head", HeadOp},
  {"tail", TailOp},
  {"rem", RemOp},
  {"print", PrintOp},
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
  Val prim = MakeSymbol(mem, "primitive");
  for (u32 i = 0; i < ArrayCount(primitives); i++) {
    Val sym = MakeSymbol(mem, primitives[i].name);
    Define(sym, MakePair(mem, prim, sym), env, mem);
  }
}
