#include "env.h"

bool Define(val var, val value, val env)
{
  val frame = Head(env);
  u32 i;
  for (i = 0; i < TupleLength(frame); i++) {
    if (TupleGet(frame, i) == 0) {
      TupleSet(frame, i, Pair(var, value));
      return true;
    }
  }
  return false;
}

val Lookup(val var, val env)
{
  while (env) {
    val frame = Head(env);
    u32 i;
    for (i = 0; i < TupleLength(frame); i++) {
      val item = TupleGet(frame, i);
      if (IsPair(item) && Head(item) == var) return Tail(item);
    }
    env = Tail(env);
  }
  return 0;
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

bool EnvSet(val value, u32 index, val env)
{
  val frame = Head(env);
  if (index >= TupleLength(frame)) return false;
  TupleSet(frame, index, value);
  return true;
}

val EnvGet(u32 index, val env)
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
