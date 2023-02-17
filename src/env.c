#include "env.h"
#include "mem.h"
#include "printer.h"

Val ExtendEnv(VM *vm, Val env)
{
  return MakePair(&vm->image->heap, nil, env);
}

Val ParentEnv(VM *vm, Val env)
{
  return Tail(vm->image->heap, env);
}

void Define(VM *vm, Val var, Val val, Val env)
{
  Val frame = Head(vm->image->heap, env);

  if (IsNil(frame)) {
    Val pair = MakePair(&vm->image->heap, var, val);
    frame = MakePair(&vm->image->heap, pair, nil);
    SetHead(&vm->image->heap, env, frame);
    return;
  }

  while (!IsNil(Tail(vm->image->heap, frame))) {
    frame = Tail(vm->image->heap, frame);
    Val pair = Head(vm->image->heap, frame);
    if (Eq(Head(vm->image->heap, pair), var)) {
      SetTail(&vm->image->heap, pair, val);
      return;
    }
  }

  Val pair = Head(vm->image->heap, frame);
  if (Eq(Head(vm->image->heap, pair), var)) {
    SetTail(&vm->image->heap, pair, val);
  } else {
    Val pair = MakePair(&vm->image->heap, var, val);
    Val item = MakePair(&vm->image->heap, pair, nil);
    SetTail(&vm->image->heap, frame, item);
  }
}

Result Lookup(VM *vm, Val var, Val env)
{
  while (!IsNil(env)) {
    Val frame = Head(vm->image->heap, env);
    while (!IsNil(frame)) {
      Val pair = Head(vm->image->heap, frame);
      if (Eq(Head(vm->image->heap, pair), var)) {
        Result result = {Ok, Tail(vm->image->heap, pair)};
        return result;
      }
      frame = Tail(vm->image->heap, frame);
    }
    env = Tail(vm->image->heap, env);
  }

  Result result = {Error, nil};
  return result;
}

Val FrameMap(VM *vm, Val env)
{
  Val frame = Head(vm->image->heap, env);
  u32 count = ListLength(vm->image->heap, frame);
  Val map = MakeMap(&vm->image->heap, count);

  while (!IsNil(frame)) {
    Val pair = Head(vm->image->heap, frame);
    Val var = Head(vm->image->heap, pair);
    Val val = Tail(vm->image->heap, pair);
    MapPut(vm->image->heap, map, var, val);
    frame = Tail(vm->image->heap, frame);
  }

  return map;
}

void PrintEnv(VM *vm)
{
  printf("─────╴Environment╶──────\n");

  Val env = vm->image->env;

  u32 frame_num = 0;

  while (!IsNil(env)) {
    Val frame = Head(vm->image->heap, env);
    while (!IsNil(frame)) {
      Val pair = Head(vm->image->heap, frame);
      Val var = Head(vm->image->heap, pair);
      Val val = Tail(vm->image->heap, pair);
      PrintVal(vm->image->heap, &vm->image->strings, var);
      printf(": ");
      PrintVal(vm->image->heap, &vm->image->strings, val);
      printf("\n");

      frame = Tail(vm->image->heap, frame);
    }
    env = Tail(vm->image->heap, env);
    frame_num++;
    printf("────────╴e%03d╶──────────\n", RawObj(env));
  }
}
