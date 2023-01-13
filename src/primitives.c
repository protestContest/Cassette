#include "primitives.h"
#include "mem.h"
#include "env.h"
#include "printer.h"
#include "value.h"
#include "reader.h"
#include "eval.h"

typedef Val (*PrimitiveFn)(Val args);

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

  if (IsDict(a) && IsDict(b) && DictSize(a) == DictSize(b)) {
    for (u32 i = 0; i < DictSize(a); i++) {
      if (!IsEqual(DictKeyAt(a, i), DictKeyAt(b, i))) return false;
      if (!IsEqual(DictValueAt(a, i), DictValueAt(b, i))) return false;
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

Val PrimHead(Val args)
{
  return Head(First(args));
}

Val PrimTail(Val args)
{
  return Tail(First(args));
}

Val PrimSetHead(Val args)
{
  Val pair = First(args);
  Val head = Second(args);
  SetHead(pair, head);
  return nil;
}

Val PrimSetTail(Val args)
{
  Val pair = First(args);
  Val tail = Second(args);
  SetTail(pair, tail);
  return nil;
}

Val PrimPair(Val args)
{
  Val head = First(args);
  Val tail = Second(args);
  return MakePair(head, tail);
}

Val PrimList(Val args)
{
  return args;
}

Val PrimMakeTuple(Val args)
{
  return ListToTuple(args);
}

Val PrimMakeDict(Val args)
{
  Val keys = First(args);
  Val vals = Second(args);
  return MakeDict(keys, vals);
}

Val PrimAccess(Val args)
{
  Val subject = First(args);
  Val index = Second(args);

  if (IsTuple(subject)) {
    if (!IsInt(index)) Error("Can only access tuples with integers");
    return TupleAt(subject, RawVal(index));
  } else if (IsDict(subject)) {
    if (!IsSym(index)) Error("Can only access dictionaries with symbols");
    return DictGet(subject, index);
  } else if (IsList(subject)) {
    if (!IsInt(index)) Error("Can only access lists with integers");
    return ListAt(subject, RawVal(index));
  } else {
    Error("Can't access this");
  }
}

Val PrimAdd(Val args)
{
  u32 sum = 0;
  while (!IsNil(args)) {
    Val arg = Head(args);
    if (IsNum(arg)) {
      float fsum = (float)sum;
      while (!IsNil(args)) {
        Val arg = Head(args);
        fsum += RawVal(arg);
        args = Tail(args);
      }
      return NumVal(fsum);

    }
    sum += RawVal(arg);
    args = Tail(args);
  }
  return IntVal(sum);
}

Val PrimSub(Val args)
{
  if (IsNum(Head(args))) {
    float sum = RawVal(Head(args));
    args = Tail(args);
    while (!IsNil(args)) {
      Val arg = Head(args);
      sum -= RawVal(arg);
      args = Tail(args);
    }
    return NumVal(sum);
  }

  u32 sum = RawVal(Head(args));
  args = Tail(args);

  while (!IsNil(args)) {
    Val arg = Head(args);
    if (IsNum(arg)) {
      float fsum = (float)sum - RawVal(arg);
      args = Tail(args);
      while (!IsNil(args)) {
        Val arg = Head(args);
        fsum -= RawVal(arg);
        args = Tail(args);
      }
      return NumVal(fsum);
    }
    sum -= RawVal(arg);
    args = Tail(args);
  }
  return IntVal(sum);
}

Val PrimMult(Val args)
{
  u32 prod = 1;

  while (!IsNil(args)) {
    Val arg = Head(args);
    if (IsNum(arg)) {
      float fprod = (float)prod;
      while (!IsNil(args)) {
        Val arg = Head(args);
        fprod *= RawVal(arg);
        args = Tail(args);
      }
      return NumVal(fprod);
    }
    prod *= RawVal(arg);
    args = Tail(args);
  }
  return IntVal(prod);
}

Val PrimDiv(Val args)
{
  Val num = First(args);
  Val den = Second(args);
  float n = RawVal(num) / RawVal(den);
  return NumVal(n);
}

Val PrimEquals(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  return BoolSymbol(IsEqual(a, b));
}

Val PrimNotEquals(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  return BoolSymbol(!IsEqual(a, b));
}

Val PrimLessThan(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  return BoolSymbol(IsLess(a, b));
}

Val PrimGreaterThan(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  return BoolSymbol(!IsLess(a, b) && !IsEqual(a, b));
}

Val PrimLessEquals(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  return BoolSymbol(IsLess(a, b) || IsEqual(a, b));
}

Val PrimGreaterEquals(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  return BoolSymbol(!IsLess(a, b));
}

Val PrimRange(Val args)
{
  Val from = First(args);
  Val to = Second(args);
  Val list = nil;

  if (RawVal(from) > RawVal(to)) {
    for (i32 i = RawVal(from); i > RawVal(to); i--) {
      list = MakePair(IntVal(i), list);
    }
    return Reverse(list);
  } else {
    for (i32 i = RawVal(from); i < RawVal(to); i++) {
      list = MakePair(IntVal(i), list);
    }
    return Reverse(list);
  }
}

Val PrimAnd(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  return BoolSymbol(IsTrue(a) && IsTrue(b));
}

Val PrimOr(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  return BoolSymbol(IsTrue(a) || IsTrue(b));
}

Val PrimNot(Val args)
{
  Val a = First(args);
  return BoolSymbol(!IsTrue(a));
}

Val PrimRem(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  return IntVal((u32)RawVal(a) % (u32)RawVal(b));
}

Val PrimEq(Val args)
{
  Val a = First(args);
  Val b = Second(args);
  if (Eq(a, b)) return MakeSymbol("true");
  else return MakeSymbol("false");
}

Val PrimDisplay(Val args)
{
  while (!IsNil(args)) {
    PrintVal(Head(args));
    args = Tail(args);
  }
  return nil;
}

Val PrimReverse(Val args)
{
  Val list = First(args);
  PrintVal(args);
  return Reverse(list);
}

Val PrimEval(Val args)
{
  Val exp = First(args);
  Val env = Second(args);
  return EvalIn(exp, env);
}

Val PrimReadFile(Val args)
{
  // char *path = BinToCStr(Head(args));
  // return ReadFile(path);
  return nil;
}

PrimitiveDef primitives[] = {
  {"head",        &PrimHead},
  {"tail",        &PrimTail},
  {"set-head!",   &PrimSetHead},
  {"set-tail!",   &PrimSetTail},
  {"list",        &PrimList},
  {"tuple",       &PrimMakeTuple},
  {"dict",        &PrimMakeDict},
  {"get",         &PrimAccess},
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
  {"eval",        &PrimEval},
  {"read-file",   &PrimReadFile}
};

void DefinePrimitives(Val env)
{
  u32 n = sizeof(primitives)/sizeof(PrimitiveDef);
  for (u32 i = 0; i < n; i++) {
    char *name = primitives[i].name;
    Val sym = MakeSymbol(name);
    Define(sym, MakePair(MakeSymbol("prim"), sym), env);
  }
}

Val DoPrimitive(Val name, Val args)
{
  u32 n = sizeof(primitives)/sizeof(PrimitiveDef);
  for (u32 i = 0; i < n; i++) {
    if (Eq(name, SymbolFor(primitives[i].name))) {
      return primitives[i].impl(args);
    }
  }

  Error("Not a primitive: \"%s\"", SymbolName(name));
}
