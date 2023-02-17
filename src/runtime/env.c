#include "env.h"
#include "mem.h"
#include "print.h"
#include "../console.h"

Val ExtendEnv(Val **mem, Val env)
{
  return MakePair(mem, nil, env);
}

Val ParentEnv(Val *mem, Val env)
{
  return Tail(mem, env);
}

void Define(Val **mem, Val var, Val val, Val env)
{
  Val frame = Head(*mem, env);

  if (IsNil(frame)) {
    Val pair = MakePair(mem, var, val);
    frame = MakePair(mem, pair, nil);
    SetHead(mem, env, frame);
    return;
  }

  while (!IsNil(Tail(*mem, frame))) {
    frame = Tail(*mem, frame);
    Val pair = Head(*mem, frame);
    if (Eq(Head(*mem, pair), var)) {
      SetTail(mem, pair, val);
      return;
    }
  }

  Val pair = Head(*mem, frame);
  if (Eq(Head(*mem, pair), var)) {
    SetTail(mem, pair, val);
  } else {
    Val pair = MakePair(mem, var, val);
    Val item = MakePair(mem, pair, nil);
    SetTail(&*mem, frame, item);
  }
}

Result Lookup(Val *mem, Val var, Val env)
{
  while (!IsNil(env)) {
    Val frame = Head(mem, env);
    while (!IsNil(frame)) {
      Val pair = Head(mem, frame);
      if (Eq(Head(mem, pair), var)) {
        Result result = {Ok, Tail(mem, pair)};
        return result;
      }
      frame = Tail(mem, frame);
    }
    env = Tail(mem, env);
  }

  Result result = {Error, nil};
  return result;
}

Val FrameMap(Val *mem, Val env)
{
  Val frame = Head(mem, env);
  u32 count = ListLength(mem, frame);
  Val map = MakeMap(&mem, count);

  while (!IsNil(frame)) {
    Val pair = Head(mem, frame);
    Val var = Head(mem, pair);
    Val val = Tail(mem, pair);
    MapPut(mem, map, var, val);
    frame = Tail(mem, frame);
  }

  return map;
}

void PrintEnv(Val *mem, StringMap *strings, Val env)
{
  Print("─────╴Environment╶──────\n");

  u32 frame_num = 0;

  while (!IsNil(env)) {
    Val frame = Head(mem, env);
    while (!IsNil(frame)) {
      Val pair = Head(mem, frame);
      Val var = Head(mem, pair);
      Val val = Tail(mem, pair);
      PrintVal(mem, strings, var);
      Print(": ");
      PrintVal(mem, strings, val);
      Print("\n");

      frame = Tail(mem, frame);
    }
    env = Tail(mem, env);
    frame_num++;
    Print("────────╴e");
    PrintInt(RawObj(env), 3);
    Print("╶──────────\n");
  }
}
