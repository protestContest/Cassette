#include "env.h"
#include "primitives.h"

Val InitialEnv(Mem *mem)
{
  MakeSymbol(mem, "ok");
  MakeSymbol(mem, "error");
  MakeSymbol(mem, "__undefined__");

  Val env = MakePair(mem, nil, nil);
  Define(MakeSymbol(mem, "nil"), nil, env, mem);
  Define(MakeSymbol(mem, "true"), SymbolFor("true"), env, mem);
  Define(MakeSymbol(mem, "false"), SymbolFor("false"), env, mem);
  return MakePair(mem, nil, env);
}

void Define(Val var, Val value, Val env, Mem *mem)
{
  Assert(!IsNil(env));

  Val frame = Head(mem, env);
  if (IsNil(frame)) {
    Val item = MakePair(mem, var, value);
    Val frame = MakePair(mem, item, nil);
    SetHead(mem, env, frame);
    return;
  }

  Val item = Head(mem, frame);
  if (Eq(Head(mem, item), var)) {
    SetTail(mem, item, value);
    return;
  }
  while (!IsNil(Tail(mem, frame))) {
    frame = Tail(mem, frame);

    Val item = Head(mem, frame);
    if (Eq(Head(mem, item), var)) {
      SetTail(mem, item, value);
      return;
    }
  }

  item = MakePair(mem, var, value);
  SetTail(mem, frame, MakePair(mem, item, nil));
}

Val Lookup(Val var, Val env, Mem *mem)
{
  while (!IsNil(env)) {
    Val frame = Head(mem, env);
    while (!IsNil(frame)) {
      Val item = Head(mem, frame);
      if (Eq(var, Head(mem, item))) {
        return Tail(mem, item);
      }
      frame = Tail(mem, frame);
    }

    env = Tail(mem, env);
  }

  return SymbolFor("__undefined__");
}

Val LookupPosition(Val var, Val env, Mem *mem)
{
  u32 frames = 0;
  while (!IsNil(env)) {
    u32 entries = 0;
    Val frame = Head(mem, env);
    while (!IsNil(frame)) {
      Val item = Head(mem, frame);
      if (Eq(var, Head(mem, item))) {
        return MakePair(mem, IntVal(frames), IntVal(entries));
      }
      entries++;
      frame = Tail(mem, frame);
    }
    frames++;
    env = Tail(mem, env);
  }

  return nil;
}

Val LookupByPosition(Val frame_num, Val entry_num, Val env, Mem *mem)
{
  u32 frames = RawInt(frame_num);
  u32 entries = RawInt(entry_num);

  for (u32 i = 0; i < frames; i++) {
    if (IsNil(env)) {
      return SymbolFor("__undefined__");
    }
    env = Tail(mem, env);
  }

  Val frame = Head(mem, env);
  for (u32 i = 0; i < entries; i++) {
    if (IsNil(frame)) {
      return SymbolFor("__undefined__");
    }
    frame = Tail(mem, frame);
  }

  Val item = Head(mem, frame);
  return Tail(mem, item);
}

void PrintEnv(Val env, Mem *mem)
{
  if (IsNil(env)) Print("<empty env>");
  while (!IsNil(env)) {
    Val frame = Head(mem, env);
    while (!IsNil(frame)) {
      Val item = Head(mem, frame);
      Val var = Head(mem, item);
      // Val val = Tail(mem, item);

      PrintSymbol(mem, var);
      // Print(": ");
      // PrintVal(mem, val);
      Print(" ");

      frame = Tail(mem, frame);
    }
    if (!IsNil(Tail(mem, env))) Print("| ");

    env = Tail(mem, env);
  }
  Print("\n");
}
