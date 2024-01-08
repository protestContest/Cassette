#include "env.h"
#include "primitives.h"
#include "mem/symbols.h"
#include "univ/system.h"

Val InitialEnv(Mem *mem)
{
  u32 num_primitives = NumPrimitives();
  Val frame = MakeTuple(num_primitives, mem);
  u32 i;

  for (i = 0; i < num_primitives; i++) {
    Val primitive = PrimVal(i);
    TupleSet(frame, i, primitive, mem);
  }

  return ExtendEnv(Nil, frame, mem);
}

void Define(Val value, u32 index, Val env, Mem *mem)
{
  Val frame;
  Assert(env != Nil);
  frame = Head(env, mem);
  if (index < TupleCount(frame, mem)) {
    TupleSet(frame, index, value, mem);
  } else {
    Define(value, index - TupleCount(frame, mem), Tail(env, mem), mem);
  }
}

Val Lookup(u32 index, Val env, Mem *mem)
{
  u32 cur = 0;

  while (env != Nil) {
    Val frame = Head(env, mem);
    u32 frame_size = TupleCount(frame, mem);

    if (cur + frame_size > index) {
      return TupleGet(frame, index - cur, mem);
    }

    cur += frame_size;
    env = Tail(env, mem);
  }
  return Undefined;
}
