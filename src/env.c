#include "env.h"

bool Define(val var, u32 index, val env)
{
  val frame = Head(env);
  if (index < 0 || index >= TupleLength(frame)) return false;
  TupleSet(frame, index, var);
  return true;
}

i32 FindEnv(val var, val env)
{
  u32 i, n = 0;

  while (env) {
    val frame = Head(env);
    env = Tail(env);
    for (i = 0; i < TupleLength(frame); i++) {
      if (var == TupleGet(frame, i)) {
        return n + i;
      }
    }
    n += TupleLength(frame);
  }
  return -1;
}

val Lookup(u32 index, val env)
{
  while (env) {
    val frame = Head(env);
    if (index < TupleLength(frame)) {
      return TupleGet(frame, index);
    }
    index -= TupleLength(frame);
    env = Tail(env);
  }
  return 0;
}
