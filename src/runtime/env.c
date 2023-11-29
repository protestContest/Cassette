#include "env.h"
#include "univ/system.h"
#include "mem/symbols.h"

void Define(Val value, u32 index, Val env, Mem *mem)
{
  Assert(env != Nil);
  TupleSet(Head(env, mem), index, value, mem);
}

Val Lookup(u32 index, Val env, Mem *mem)
{
  u32 cur = 0;

  while (env != Nil) {
    Val frame = Head(env, mem);
    u32 frame_size = TupleLength(frame, mem);

    if (cur + frame_size > index) {
      return TupleGet(frame, index - cur, mem);
    }

    cur += frame_size;
    env = Tail(env, mem);
  }
  return Undefined;
}

i32 FindDefinition(Val var, Val env, Mem *mem)
{
  u32 index = 0;

  /* don't look in the top frame, which where modules are defined */
  while (Tail(env, mem) != Nil) {
    Val frame = Head(env, mem);
    u32 i;
    for (i = 0; i < TupleLength(frame, mem); i++) {
      /* search from the back of the frame, since those are defined later and
         may shadow an earlier variable of the same name */
      u32 pos = (TupleLength(frame, mem) - i) - 1;
      if (TupleGet(frame, pos, mem) == var) {
        return index + pos;
      }
    }
    index += TupleLength(frame, mem);
    env = Tail(env, mem);
  }

  return -1;
}

i32 FindModule(Val mod, Val env, Mem *mem)
{
  u32 index = 0;
  u32 i;
  Val frame;

  /* skip to the top frame, where modules are defined */
  while (Tail(env, mem) != Nil) {
    Val frame = Head(env, mem);
    index += TupleLength(frame, mem);
    env = Tail(env, mem);
  }

  frame = Head(env, mem);
  for (i = 0; i < TupleLength(frame, mem); i++) {
    u32 pos = (TupleLength(frame, mem) - i) - 1;
    if (TupleGet(frame, pos, mem) == mod) {
      return index + pos;
    }
  }

  return -1;
}
