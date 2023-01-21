#include "primitives.h"
#include "base.h"
#include "mem.h"
#include "env.h"
#include "printer.h"
#include "value.h"
#include "reader.h"
#include "eval.h"

typedef EvalResult (*PrimitiveFn)(Val args);

typedef struct {
  char *name;
  PrimitiveFn impl;
} PrimitiveDef;

bool IsTrue(Val val)
{
  return !IsNil(val) && !Eq(val, SymbolFor("false"));
}

bool IsEqual(Val a, Val b)
{
  if (Eq(a, b)) return true;

  if (IsNil(a) && IsNil(b)) return true;
  if (IsNumeric(a) && IsNumeric(b)) return RawVal(a) == RawVal(b);
  if (IsSym(a) && IsSym(b)) return RawVal(a) == RawVal(b);

  if (IsPair(a) && IsPair(b)) {
    return IsEqual(Head(a), Head(b)) && IsEqual(Tail(a), Tail(b));
  }

  if (IsBin(a) && IsBin(b) && BinaryLength(a) == BinaryLength(b)) {
    char *data_a = BinaryData(a);
    char *data_b = BinaryData(b);
    for (u32 i = 0; i < BinaryLength(a); i++) {
      if (data_a[i] != data_b[i]) return false;
    }
    return true;
  }

  if (IsTuple(a) && IsTuple(b) && TupleLength(a) == TupleLength(b)) {
    for (u32 i = 0; i < TupleLength(a); i++) {
      if (!IsEqual(TupleAt(a, i), TupleAt(b, i))) return false;
    }
    return true;
  }

  return false;
}

bool IsLess(Val a, Val b)
{
  if (IsNil(a) && IsNil(b)) return false;
  if (IsNumeric(a) && IsNumeric(b)) return RawVal(a) < RawVal(b);

  if (IsSym(a) && IsSym(b)) {
    return strcmp(SymbolName(a), SymbolName(b)) < 0;
  }

  if (IsPair(a) && IsPair(b)) {
    if (IsNil(a)) return true;
    if (IsNil(b)) return false;

    if (IsEqual(Head(a), Head(b))) return IsLess(Tail(a), Tail(b));

    return IsLess(Head(a), Head(b));
  }

  if (IsBin(a) && IsBin(b)) {
    char *data_a = BinaryData(a);
    char *data_b = BinaryData(b);
    for (u32 i = 0; i < BinaryLength(a) && i < BinaryLength(b); i++) {
      if (data_a[i] < data_b[i]) return true;
      if (data_a[i] > data_b[i]) return false;
    }
    return BinaryLength(a) < BinaryLength(b);
  }

  if (IsTuple(a) && IsTuple(b)) {
    if (TupleLength(a) == TupleLength(b)) {
      for (u32 i = 0; i < TupleLength(a); i++) {
        if (IsLess(TupleAt(a, i), TupleAt(b, i))) return true;
      }
      return false;
    }
    return TupleLength(a) < TupleLength(b);
  }

  return false;
}

EvalResult PrimHead(Val args)
{
  Val arg = First(args);
  if (!IsPair(arg)) {
    char *msg = NULL;
    PrintInto(msg, "Argument error: not a pair: %s", ValStr(arg));
    return RuntimeError(msg);
  }

  return EvalOk(Head(arg));
}

EvalResult PrimTail(Val args)
{
  Val arg = First(args);
  if (!IsPair(arg)) {
    char *msg = NULL;
    PrintInto(msg, "Argument error: not a pair: %s", ValStr(arg));
    return RuntimeError(msg);
  }

  return EvalOk(Tail(arg));
}

EvalResult PrimPair(Val args)
{
  Val head = First(args);
  Val tail = Second(args);
  return EvalOk(MakePair(head, tail));
}

EvalResult PrimList(Val args)
{
  return EvalOk(args);
}

EvalResult PrimMakeTuple(Val args)
{
  return EvalOk(ListToTuple(args));
}

EvalResult PrimMakeDict(Val args)
{
  Val keys = First(args);
  Val vals = Second(args);
  return EvalOk(MakeDict(keys, vals));
}

EvalResult PrimAdd(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  if (!IsNumeric(a) || !IsNumeric(b)) {
    char *msg = NULL;
    PrintInto(msg, "Arithmetic error: %s + %s", ValStr(a), ValStr(b));
    return RuntimeError(msg);
  }

  if (IsInt(a) && IsInt(b)) {
    return EvalOk(IntVal((u32)RawVal(a) + (u32)RawVal(b)));
  } else {
    return EvalOk(NumVal(RawVal(a) + RawVal(b)));
  }
}

EvalResult PrimSub(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  if (!IsNumeric(a) || !IsNumeric(b)) {
    char *msg = NULL;
    PrintInto(msg, "Arithmetic error: %s - %s", ValStr(a), ValStr(b));
    return RuntimeError(msg);
  }

  if (IsInt(a) && IsInt(b)) {
    return EvalOk(IntVal((u32)RawVal(a) - (u32)RawVal(b)));
  } else {
    return EvalOk(NumVal(RawVal(a) - RawVal(b)));
  }
}

EvalResult PrimMult(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  if (!IsNumeric(a) || !IsNumeric(b)) {
    char *msg = NULL;
    PrintInto(msg, "Arithmetic error: %s * %s", ValStr(a), ValStr(b));
    return RuntimeError(msg);
  }

  return EvalOk(NumVal(RawVal(a) * RawVal(b)));
}

EvalResult PrimDiv(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  if (!IsNumeric(a) || !IsNumeric(b) || RawVal(b) == 0) {
    char *msg = NULL;
    PrintInto(msg, "Arithmetic error: %s / %s", ValStr(a), ValStr(b));
    return RuntimeError(msg);
  }

  return EvalOk(NumVal(RawVal(a) / RawVal(b)));
}

EvalResult PrimEquals(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  return EvalOk(BoolSymbol(IsEqual(a, b)));
}

EvalResult PrimNotEquals(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  return EvalOk(BoolSymbol(!IsEqual(a, b)));
}

EvalResult PrimLessThan(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  return EvalOk(BoolSymbol(IsLess(a, b)));
}

EvalResult PrimGreaterThan(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  return EvalOk(BoolSymbol(!IsLess(a, b) && !IsEqual(a, b)));
}

EvalResult PrimLessEquals(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  return EvalOk(BoolSymbol(IsLess(a, b) || IsEqual(a, b)));
}

EvalResult PrimGreaterEquals(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  return EvalOk(BoolSymbol(!IsLess(a, b)));
}

EvalResult PrimRange(Val args)
{
  Val from = First(args);
  Val to = Second(args);
  Val list = nil;

  if (!IsInt(from) || !IsInt(to)) {
    char *msg = NULL;
    PrintInto(msg, "Argument error: %s .. %s", ValStr(from), ValStr(to));
    return RuntimeError(msg);
  }

  if (RawVal(from) > RawVal(to)) {
    for (i32 i = RawVal(from); i > RawVal(to); i--) {
      list = MakePair(IntVal(i), list);
    }
    return EvalOk(Reverse(list));
  } else {
    for (i32 i = RawVal(from); i < RawVal(to); i++) {
      list = MakePair(IntVal(i), list);
    }
    return EvalOk(Reverse(list));
  }
}

EvalResult PrimAnd(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  return EvalOk(BoolSymbol(IsTrue(a) && IsTrue(b)));
}

EvalResult PrimOr(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  return EvalOk(BoolSymbol(IsTrue(a) || IsTrue(b)));
}

EvalResult PrimNot(Val args)
{
  Val a = First(args);
  return EvalOk(BoolSymbol(!IsTrue(a)));
}

EvalResult PrimRem(Val args)
{
  Val a = First(args);
  Val b = Second(args);

  if (!IsInt(a) || !IsInt(b)) {
    char *msg = NULL;
    PrintInto(msg, "Argument error: (rem %s %s)", ValStr(a), ValStr(b));
    return RuntimeError(msg);
  }

  return EvalOk(IntVal((u32)RawVal(a) % (u32)RawVal(b)));
}

EvalResult PrimEq(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  if (Eq(a, b)) return EvalOk(MakeSymbol("true"));
  else return EvalOk(MakeSymbol("false"));
}

EvalResult PrimDisplay(Val args)
{
  while (!IsNil(args)) {
    PrintVal(Head(args));
    args = Tail(args);
  }
  return EvalOk(nil);
}

EvalResult PrimReverse(Val args)
{
  Val list = First(args);
  return EvalOk(Reverse(list));
}

// EvalResult PrimEval(Val args)
// {
//   Val exp = First(args);
//   Val env = Second(args);
//   return Eval(exp, env);
// }

// EvalResult PrimReadFile(Val args)
// {
//   // char *path = BinToCStr(Head(args));
//   // return ReadFile(path);
//   return nil;
// }

PrimitiveDef primitives[] = {
  {"head",        &PrimHead},
  {"tail",        &PrimTail},
  {"list",        &PrimList},
  {"tuple",       &PrimMakeTuple},
  {"dict",        &PrimMakeDict},
  {"|",           &PrimPair},
  {"+",           &PrimAdd},
  {"-",           &PrimSub},
  {"*",           &PrimMult},
  {"/",           &PrimDiv},
  {"=",           &PrimEquals},
  {"!=",          &PrimNotEquals},
  {"<",           &PrimLessThan},
  {">",           &PrimGreaterThan},
  {"<=",          &PrimLessEquals},
  {">=",          &PrimGreaterEquals},
  {"..",          &PrimRange},
  {"and",         &PrimAnd},
  {"or",          &PrimOr},
  {"not",         &PrimNot},
  {"rem",         &PrimRem},
  {"eq?",         &PrimEq},
  {"display",     &PrimDisplay},
  {"reverse",     &PrimReverse},
  // {"eval",        &PrimEval},
  // {"read-file",   &PrimReadFile}
};


Val Primitives(void)
{
  Val names = nil;
  Val vals = nil;
  for (u32 i = 0; i < ArrayCount(primitives); i++) {
    Val symbol = MakeSymbol(primitives[i].name);
    names = MakePair(symbol, names);
    Val val = MakePair(MakeSymbol("primitive"), symbol);
    vals = MakePair(val, vals);
  }
  return MakePair(names, vals);
}

void DefinePrimitives(Val env)
{
  // u32 n = sizeof(primitives)/sizeof(PrimitiveDef);
  // for (u32 i = 0; i < n; i++) {
  //   char *name = primitives[i].name;
  //   Val sym = MakeSymbol(name);
  //   Define(sym, MakePair(MakeSymbol("prim"), sym), env);
  // }
}

EvalResult DoPrimitive(Val name, Val args)
{
  u32 n = sizeof(primitives)/sizeof(PrimitiveDef);
  for (u32 i = 0; i < n; i++) {
    if (Eq(name, SymbolFor(primitives[i].name))) {
      return primitives[i].impl(args);
    }
  }

  char *msg = NULL;
  PrintInto(msg, "Not a primitive: \"%s\"", SymbolName(name));
  return RuntimeError(msg);
}
