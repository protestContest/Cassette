#include "primitives.h"
#include "mem.h"
#include "env.h"
#include "printer.h"
#include "value.h"
#include "eval.h"
#include "reader.h"

typedef Val (*PrimitiveFn)(Val args);

typedef struct {
  char *name;
  PrimitiveFn impl;
} PrimitiveDef;

Val PrimHead(Val args)
{
  return Head(Head(args));
}

Val PrimTail(Val args)
{
  return Tail(Head(args));
}

Val PrimSetHead(Val args)
{
  Val pair = Head(args);
  Val head = Head(Tail(args));
  SetHead(pair, head);
  return MakeSymbol("ok", 2);
}

Val PrimSetTail(Val args)
{
  Val pair = Head(args);
  Val tail = Head(Tail(args));
  SetTail(pair, tail);
  return MakeSymbol("ok", 2);
}

Val PrimPair(Val args)
{
  Val head = Head(args);
  Val tail = Head(Tail(args));
  return MakePair(head, tail);
}

Val PrimMakeTuple(Val args)
{
  return ListToTuple(args);
}

Val PrimMakeDict(Val args)
{
  return MakeDict(args);
}

Val PrimNth(Val args)
{
  Val tuple = Head(args);
  Val index = Head(Tail(args));
  return TupleAt(tuple, RawVal(index));
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
  Val num = Head(args);
  Val den = Head(Tail(args));
  float n = RawVal(num) / RawVal(den);
  return NumVal(n);
}

Val PrimNumEquals(Val args)
{
  Val a = Head(args);
  Val b = Head(Tail(args));
  if ((IsInt(a) && IsInt(b)) || (IsNum(a) && IsNum(b))) {
    if (RawVal(a) == RawVal(b)) {
      return MakeSymbol("true", 4);
    } else {
      return MakeSymbol("false", 5);
    }
  } else if ((IsInt(a) && IsNum(b)) || (IsNum(a) && IsInt(b))) {
    if ((float)RawVal(a) == (float)RawVal(b)) {
      return MakeSymbol("true", 4);
    } else {
      return MakeSymbol("false", 5);
    }
  } else {
    return MakeSymbol("false", 5);
  }
}

Val PrimRem(Val args)
{
  Val a = Head(args);
  Val b = Head(Tail(args));
  return IntVal((u32)RawVal(a) % (u32)RawVal(b));
}

Val PrimEq(Val args)
{
  Val a = Head(args);
  Val b = Head(Tail(args));
  if (Eq(a, b)) return MakeSymbol("true", 4);
  else return MakeSymbol("false", 5);
}

Val PrimDisplay(Val args)
{
  while (!IsNil(args)) {
    PrintVal(Head(args));
    args = Tail(args);
  }
  return MakeSymbol("ok", 2);
}

Val PrimEval(Val args)
{
  Val exp = Head(args);
  Val env = Head(Tail(args));
  return Eval(exp, env);
}

Val PrimReadFile(Val args)
{
  char *path = BinToCStr(Head(args));
  return ReadFile(path);
}

#define NUM_PRIMITIVES 18
PrimitiveDef primitives[NUM_PRIMITIVES] = {
  {"head",        &PrimHead},
  {"tail",        &PrimTail},
  {"set-head!",   &PrimSetHead},
  {"set-tail!",   &PrimSetTail},
  {"make-pair",   &PrimPair},
  {"make-tuple",  &PrimMakeTuple},
  {"make-dict",   &PrimMakeDict},
  {"nth",         &PrimNth},
  {"+",           &PrimAdd},
  {"-",           &PrimSub},
  {"*",           &PrimMult},
  {"/",           &PrimDiv},
  {"=",           &PrimNumEquals},
  {"rem",         &PrimRem},
  {"eq?",         &PrimEq},
  {"display",     &PrimDisplay},
  {"eval",        &PrimEval},
  {"read-file",   &PrimReadFile}
};

void DefinePrimitives(Val env)
{
  for (u32 i = 0; i < NUM_PRIMITIVES; i++) {
    char *name = primitives[i].name;
    Val sym = MakeSymbol(name, strlen(name));
    Define(sym, MakePair(MakeSymbol("prim", 4), sym), env);
  }
}

Val DoPrimitive(Val name, Val args)
{
  for (u32 i = 0; i < NUM_PRIMITIVES; i++) {
    if (Eq(name, SymbolFor(primitives[i].name))) {
      return primitives[i].impl(args);
    }
  }

  Error("Not a primitive: %s", SymbolName(name));
}
