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

void PrintEnv(Val env, Mem *mem, SymbolTable *symbols)
{
  printf("Env:\n");
  while (env != Nil) {
    Val frame = Head(env, mem);
    u32 i;
    printf("- ");
    for (i = 0; i < TupleLength(frame, mem); i++) {
      Val item = TupleGet(frame, i, mem);
      PrintVal(item, symbols);
      printf(" ");
    }

    printf("\n");
    env = Tail(env, mem);
  }
}
