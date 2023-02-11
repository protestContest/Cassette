#include "env.h"
#include "mem.h"

Val ExtendEnv(VM *vm, Val env)
{
  return MakePair(&vm->heap, nil, env);
}

Val ParentEnv(VM *vm, Val env)
{
  return Tail(vm->heap, env);
}

void Define(VM *vm, Val var, Val val, Val env)
{
  Val frame = Head(vm->heap, env);

  if (IsNil(frame)) {
    Val pair = MakePair(&vm->heap, var, val);
    frame = MakePair(&vm->heap, pair, nil);
    SetHead(&vm->heap, env, frame);
    return;
  }

  while (!IsNil(Tail(vm->heap, frame))) {
    frame = Tail(vm->heap, frame);
    Val pair = Head(vm->heap, frame);
    if (Eq(Head(vm->heap, pair), var)) {
      SetTail(&vm->heap, pair, val);
      return;
    }
  }

  Val pair = Head(vm->heap, frame);
  if (Eq(Head(vm->heap, pair), var)) {
    SetTail(&vm->heap, pair, val);
  } else {
    Val pair = MakePair(&vm->heap, var, val);
    Val item = MakePair(&vm->heap, pair, nil);
    SetTail(&vm->heap, frame, item);
  }
}

Result Lookup(VM *vm, Val var, Val env)
{
  while (!IsNil(env)) {
    Val frame = Head(vm->heap, env);
    while (!IsNil(frame)) {
      Val pair = Head(vm->heap, frame);
      if (Eq(Head(vm->heap, pair), var)) {
        Result result = {Ok, Tail(vm->heap, pair)};
        return result;
      }
      frame = Tail(vm->heap, frame);
    }
    env = Tail(vm->heap, env);
  }

  Result result = {Error, nil};
  return result;
}

Val FrameMap(VM *vm, Val env)
{
  Val frame = Head(vm->heap, env);
  u32 count = ListLength(vm->heap, frame);
  Val map = MakeMap(&vm->heap, count);

  while (!IsNil(frame)) {
    Val pair = Head(vm->heap, frame);
    Val var = Head(vm->heap, pair);
    Val val = Tail(vm->heap, pair);
    MapPut(vm->heap, map, var, val);
    frame = Tail(vm->heap, frame);
  }

  return map;
}

void PrintEnv(VM *vm)
{
  printf("─────╴Environment╶──────\n");

  Val env = vm->env;

  u32 frame_num = 0;

  while (!IsNil(env)) {
    Val frame = Head(vm->heap, env);
    while (!IsNil(frame)) {
      Val pair = Head(vm->heap, frame);
      Val var = Head(vm->heap, pair);
      Val val = Tail(vm->heap, pair);
      PrintVMVal(vm, var);
      printf(": ");
      PrintVMVal(vm, val);
      printf("\n");

      frame = Tail(vm->heap, frame);
    }
    env = Tail(vm->heap, env);
    frame_num++;
    printf("────────╴e%03d╶──────────\n", RawObj(env));
  }
}
