#include "env.h"
#include <stdio.h>

Val ExtendEnv(Val env, Val frame, Mem *mem)
{
  return Pair(frame, env, mem);
}

void Define(Val value, u32 index, Val env, Mem *mem)
{
  Assert(env != Nil);
  TupleSet(Head(env, mem), index, value, mem);
}

Val Lookup(u32 frames, u32 index, Val env, Mem *mem)
{
  u32 frame;

  if (env == Nil) return Undefined;
  for (frame = 0; frame < frames; frame++) {
    env = Tail(env, mem);
    if (env == Nil) return Undefined;
  }

  if (index >= TupleLength(Head(env, mem), mem)) return Undefined;

  return TupleGet(Head(env, mem), index, mem);
}

i32 FindDefinition(Val var, Val env, Mem *mem)
{
  u32 frame = 0;
  u32 i;

  while (env != Nil) {
    for (i = 0; i < TupleLength(Head(env, mem), mem); i++) {
      if (TupleGet(Head(env, mem), i, mem) == var) {
        return (frame << 16) | i;
      }
    }
    frame++;
    env = Tail(env, mem);
  }

  return -1;
}
