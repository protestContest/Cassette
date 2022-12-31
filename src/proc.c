#include "proc.h"
#include "mem.h"
#include "env.h"
#include "printer.h"

Val MakeProc(Val params, Val body, Val env)
{
  return MakeTuple(4, MakeSymbol("proc", 4), params, body, env);
}

Val ProcParams(Val proc)
{
  return TupleAt(proc, 1);
}

Val ProcBody(Val proc)
{
  return TupleAt(proc, 2);
}

Val ProcEnv(Val proc)
{
  return TupleAt(proc, 3);
}

void DefinePrimitive(char *name, Val env)
{
  Val sym = MakeSymbol(name, strlen(name));
  Val prim = MakeSymbol("prim", 4);
  Define(sym, MakePair(prim, sym), env);
}

Val DoPrimitive(Val name, Val args)
{
  if (Eq(name, MakeSymbol("pair", 4))) {
    Val head = Head(args);
    Val tail = Head(Tail(args));
    return MakePair(head, tail);
  }

  if (Eq(name, MakeSymbol("head", 4))) {
    return Head(Head(args));
  }

  if (Eq(name, MakeSymbol("tail", 4))) {
    return Tail(Head(args));
  }

  if (Eq(name, MakeSymbol("nth", 3))) {
    Val tuple = Head(args);
    Val index = Head(Tail(args));
    return TupleAt(tuple, RawVal(index));
  }

  if (Eq(name, MakeSymbol("+", 1))) {
    float sum = 0;
    while (!IsNil(args)) {
      sum += RawVal(Head(args));
      args = Tail(args);
    }
    return NumVal(sum);
  }

  if (Eq(name, MakeSymbol("-", 1))) {
    float sum = 0;
    while (!IsNil(args)) {
      sum -= RawVal(Head(args));
      args = Tail(args);
    }
    return NumVal(sum);
  }

  if (Eq(name, MakeSymbol("*", 1))) {
    float prod = 1;
    bool is_float = false;
    while (!IsNil(args)) {
      if (IsNum(Head(args))) {
        is_float = true;
      }
      prod *= RawVal(Head(args));
      args = Tail(args);
    }
    if (is_float) {
      return NumVal(prod);
    } else {
      return IntVal((u32)prod);
    }
  }

  if (Eq(name, MakeSymbol("/", 1))) {
    Val num = Head(args);
    Val den = Head(Tail(args));
    float n = RawVal(num) / RawVal(den);
    return NumVal(n);
  }

  if (Eq(name, MakeSymbol("=", 1))) {
    Val a = Head(args);
    Val b = Head(Tail(args));
    if (Eq(a, b)) return MakeSymbol("true", 4);
    else return MakeSymbol("false", 5);
  }

  if (Eq(name, MakeSymbol("display", 7))) {
    while (!IsNil(args)) {
      PrintVal(Head(args));
      printf("\n");
      args = Tail(args);
    }
    return MakeSymbol("ok", 2);
  }

  if (Eq(name, MakeSymbol("reverse", 7))) {
    Val list = Head(args);
    return Reverse(list);
  }

  Error("Not a primitive: %s", SymbolName(name));
}
