#include "env.h"
#include "proc.h"

Val InitialEnv(Mem *mem)
{
  Val env = Pair(MakeSymbol("ε", mem), nil, mem);
  env = ExtendEnv(env, mem);
  DefinePrimitives(env, mem);
  return ExtendEnv(env, mem);
}

Val ExtendEnv(Val env, Mem *mem)
{
  Assert(Eq(Head(env, mem), SymbolFor("ε")));

  Val frames = Pair(nil, Tail(env, mem), mem);
  return Pair(SymbolFor("ε"), frames, mem);
}

void Define(Val var, Val val, Val env, Mem *mem)
{
  Assert(Eq(Head(env, mem), SymbolFor("ε")));

  Val frames = Tail(env, mem);
  Val frame = Head(frames, mem);

  if (IsNil(frame)) {
    Val pair = Pair(var, val, mem);
    frame = Pair(pair, nil, mem);
    SetHead(frames, frame, mem);
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

  Val pair = Pair(var, val, mem);
  SetTail(frame, Pair(pair, nil, mem), mem);
}

Val Lookup(Val var, Val env, Mem *mem)
{
  Assert(Eq(Head(env, mem), SymbolFor("ε")));

  Val frames = Tail(env, mem);
  while (!IsNil(frames)) {
    Val frame = Head(frames, mem);
    while (!IsNil(frame)) {
      Val pair = Head(frame, mem);

      if (Eq(var, Head(pair, mem))) return Tail(pair, mem);

      frame = Tail(frame, mem);
    }

    frames = Tail(frames, mem);
  }

  return MakeSymbol("__UNDEFINED__", mem);
}
