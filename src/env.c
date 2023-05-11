#include "env.h"
#include "primitives.h"

Val InitialEnv(Mem *mem)
{
  MakeSymbol(mem, "ok");
  MakeSymbol(mem, "error");
  MakeSymbol(mem, "__undefined__");
  MakeSymbol(mem, "λ");

  Val env = MakePair(mem, nil, nil);
  Define(MakeSymbol(mem, "nil"), nil, env, mem);
  Define(MakeSymbol(mem, "true"), SymbolFor("true"), env, mem);
  Define(MakeSymbol(mem, "false"), SymbolFor("false"), env, mem);
  // DefinePrimitives(env, mem);
  return MakePair(mem, nil, env);
}

void Define(Val var, Val value, Val env, Mem *mem)
{
  Assert(!IsNil(env));
  Val frame = Head(mem, env);
  while (!IsNil(frame)) {
    Val item = Head(mem, frame);
    if (Eq(var, Head(mem, item))) {
      SetTail(mem, item, value);
      return;
    }
    frame = Tail(mem, frame);
  }

  Val item = MakePair(mem, var, value);
  frame = MakePair(mem, item, Head(mem, env));
  SetHead(mem, env, frame);
}

Val Lookup(Val var, Val env, Mem *mem)
{
  while (!IsNil(env)) {
    Val frame = Head(mem, env);
    while (!IsNil(frame)) {
      Val item = Head(mem, frame);
      if (Eq(var, Head(mem, item))) {
        return Tail(mem, item);
      }
      frame = Tail(mem, frame);
    }

    env = Tail(mem, env);
  }

  return SymbolFor("__undefined__");
}

Val MakeProcedure(Val body, Val env, Mem *mem)
{
  return
    MakePair(mem, SymbolFor("λ"),
    MakePair(mem, body,
    MakePair(mem, env, nil)));
}

Val ProcBody(Val proc, Mem *mem)
{
  return ListAt(mem, proc, 1);
}

Val ProcEnv(Val proc, Mem *mem)
{
  return ListAt(mem, proc, 2);
}

void PrintEnv(Val env, Mem *mem)
{
  if (IsNil(env)) Print("<empty env>");
  while (!IsNil(env)) {
    Print("----------------\n");
    Val frame = Head(mem, env);
    while (!IsNil(frame)) {
      Val item = Head(mem, frame);
      Val var = Head(mem, item);
      Val val = Tail(mem, item);

      Print(SymbolName(mem, var));
      Print(": ");
      PrintVal(mem, val);
      Print(" ");

      frame = Tail(mem, frame);
    }

    env = Tail(mem, env);
  }
}
