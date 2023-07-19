#include "env.h"
#include "proc.h"
#include "primitives.h"

Val InitialEnv(Mem *mem)
{
  Val env = ExtendEnv(nil, mem);
  DefinePrimitives(env, mem);
  return ExtendEnv(env, mem);
}

Val ExtendEnv(Val env, Mem *mem)
{
  return Pair(nil, env, mem);
}

void Define(Val var, Val val, Val env, Mem *mem)
{
  Val frame = Head(env, mem);

  if (IsNil(frame)) {
    Val pair = Pair(var, val, mem);
    frame = Pair(pair, nil, mem);
    SetHead(env, frame, mem);
    return;
  }

  Val pair = Head(frame, mem);
  if (Eq(var, Head(pair, mem))) {
    SetTail(pair, val , mem);
    return;
  }

  while (!IsNil(Tail(frame, mem))) {
    Val pair = Head(frame, mem);
    if (Eq(var, Head(pair, mem))) {
      SetTail(pair, val, mem);
      return;
    }
    frame = Tail(frame, mem);
  }

  pair = Pair(var, val, mem);
  SetTail(frame, Pair(pair, nil, mem), mem);
}

Val Lookup(Val var, Val env, Mem *mem)
{
  while (!IsNil(env)) {
    Val frame = Head(env, mem);
    while (!IsNil(frame)) {
      Val pair = Head(frame, mem);

      if (Eq(var, Head(pair, mem))) return Tail(pair, mem);

      frame = Tail(frame, mem);
    }

    env = Tail(env, mem);
  }

  return MakeSymbol("*undefined*", mem);
}

void ImportEnv(Val imports, Val env, Mem *mem)
{
  Val keys = MapKeys(imports, mem);
  Val vals = MapValues(imports, mem);

  for (u32 i = 0; i < MapCount(imports, mem); i++) {
    Val key = TupleGet(keys, i, mem);
    Val val = TupleGet(vals, i, mem);
    Define(key, val, env, mem);
  }
}

Val ExportEnv(Val env, Mem *mem)
{
  if (IsNil(env)) return nil;
  Val frame = Head(env, mem);
  u32 num_exports = ListLength(frame, mem);

  Val exports = MakeMap(num_exports, mem);
  while (!IsNil(frame)) {
    Val pair = Head(frame, mem);
    Val var = Head(pair, mem);
    Val value = Tail(pair, mem);
    MapSet(exports, var, value, mem);
    frame = Tail(frame, mem);
  }

  return exports;
}

void PrintEnv(Val env, Mem *mem)
{
  Print("┌╴Env ");
  PrintInt(RawVal(env));
  Print("╶───────────────\n");

  if (IsNil(env)) {
    Print("(empty)\n");
  }
  while (!IsNil(env)) {
    Val frame = Head(env, mem);

    while (!IsNil(frame)) {
      Print("│");
      Val pair = Head(frame, mem);
      Val var = Head(pair, mem);
      Val val = Tail(pair, mem);
      Print(SymbolName(var, mem));
      Print(": ");
      InspectVal(val, mem);
      Print("\n");
      frame = Tail(frame, mem);
    }
    if (!IsNil(Tail(env, mem))) {
      Print("├────────────────────\n");
    }


    env = Tail(env, mem);
  }
  Print("└────────────────────\n");
}

