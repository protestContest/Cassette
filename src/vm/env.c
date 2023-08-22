#include "env.h"
#include "function.h"

Val InitialEnv(Heap *mem)
{
  Val prims = GetPrimitives(mem);
  return ExtendEnv(nil, prims, mem);
}

Val CompileEnv(Heap *mem)
{
  Val prims = PrimitiveNames(mem);
  return ExtendEnv(nil, prims, mem);
}

Val ExtendEnv(Val env, Val frame, Heap *mem)
{
  return Pair(frame, env, mem);
}

Val RestoreEnv(Val env, Heap *mem)
{
  return Tail(env, mem);
}

void Define(u32 pos, Val item, Val env, Heap *mem)
{
  Val frame = Head(env, mem);
  TupleSet(frame, pos, item, mem);
}

Val Lookup(Val env, u32 fnum, u32 pos, Heap *mem)
{
  for (u32 i = 0; i < fnum; i++) env = Tail(env, mem);
  Val frame = Head(env, mem);
  if (IsNil(frame) || TupleLength(frame, mem) < pos) {
    return MakeSymbol("*undefined*", mem);
  }
  return TupleGet(frame, pos, mem);
}

Val FindVar(Val var, Val env, Heap *mem)
{
  Inspect(env, mem);
  Print("\n");
  Print("Looking for ");
  Inspect(var, mem);
  Print(": ");

  u32 fnum = 0;

  while (!IsNil(env)) {
    Val frame = Head(env, mem);
    for (u32 i = 0; i < TupleLength(frame, mem); i++) {
      Val item = TupleGet(frame, i, mem);
      if (Eq(var, item)) {
        PrintInt(fnum);
        Print(",");
        PrintInt(i);
        Print("\n");
        return Pair(IntVal(fnum), IntVal(i), mem);
      }
    }

    env = Tail(env, mem);
    fnum++;
  }

  Print("X\n");

  return nil;
}