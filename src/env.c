#include "env.h"
#include "mem.h"

Val ExtendEnv(VM *vm, Val env)
{
  return MakePair(&vm->heap, MakePair(&vm->heap, nil, nil), env);
}

void Define(VM *vm, Val var, Val val, Val env)
{
  Val frame = FirstFrame(vm, env);
  while (!IsNil(frame)) {
    Val pair = FirstPair(vm, frame);

    if (Eq(EnvVar(vm, pair), var)) {
      SetEnvVal(vm, pair, val);
      return;
    }

    frame = RestOfFrame(vm, frame);
  }

  Val pair = MakePair(&vm->heap, var, val);
  frame = MakePair(&vm->heap, pair, FirstFrame(vm, env));
  SetTail(&vm->heap, FrameHeader(vm, env), frame);
}

Result Lookup(VM *vm, Val var, Val env)
{
  while (!IsNil(env)) {
    Val frame = FirstFrame(vm, env);
    while (!IsNil(frame)) {
      Val pair = FirstPair(vm, frame);

      if (Eq(EnvVar(vm, pair), var)) {
        Result result = {Ok, EnvVal(vm, pair)};
        return result;
      }

      frame = RestOfFrame(vm, frame);
    }

    env = RestOfEnv(vm, env);
  }

  Result result = {Error, nil};
  return result;
}
