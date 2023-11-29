#include "env.h"
#include "univ/system.h"
#include "mem/symbols.h"

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
  u32 f = 0;
  u32 i;

  /* don't look in the top frame, which where modules are defined */
  while (Tail(env, mem) != Nil) {
    Val frame = Head(env, mem);
    for (i = 0; i < TupleLength(frame, mem); i++) {
      u32 pos = (TupleLength(frame, mem) - i) - 1;
      if (TupleGet(frame, pos, mem) == var) {
        return (f << 16) | pos;
      }
    }
    f++;
    env = Tail(env, mem);
  }

  return -1;
}

i32 FindModule(Val mod, Val env, Mem *mem)
{
  u32 f = 0;
  u32 i;
  u32 location = -1;

  while (env != Nil) {
    Val frame = Head(env, mem);
    for (i = 0; i < TupleLength(frame, mem); i++) {
      u32 pos = (TupleLength(frame, mem) - i) - 1;
      if (TupleGet(frame, pos, mem) == mod) {
        location = (f << 16) | pos;
      }
    }
    f++;
    env = Tail(env, mem);
  }

  return location;
}
