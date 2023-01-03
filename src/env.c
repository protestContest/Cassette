#include "env.h"
#include "mem.h"
#include "primitives.h"

Val InitialEnv(void)
{
  Val env = ExtendEnv(nil, nil, nil);
  DefinePrimitives(env);
  Define(MakeSymbol("true", 4), IntVal(1), env);
  Define(MakeSymbol("false", 5), nil, env);
  Define(MakeSymbol("nil", 3), nil, env);

  return env;
}

Val ParentEnv(Val env) {
  return Tail(env);
}

Val FirstFrame(Val env) {
  return Head(env);
}

Val MakeFrame(Val vars, Val vals) {
  return MakePair(vars, vals);
}

Val FrameVars(Val frame) {
  return Head(frame);
}

Val FrameVals(Val frame) {
  return Tail(frame);
}

void AddBinding(Val var, Val val, Val frame)
{
  SetHead(frame, MakePair(var, Head(frame)));
  SetTail(frame, MakePair(val, Tail(frame)));
}

Val ExtendEnv(Val vars, Val vals, Val env)
{
  return MakePair(MakeFrame(vars, vals), env);
}

Val Lookup(Val var, Val env)
{
  while (!IsNil(env)) {
    Val frame = FirstFrame(env);
    Val vars = FrameVars(frame);
    Val vals = FrameVals(frame);
    while (!IsNil(vars)) {
      if (Eq(var, Head(vars))) {
        return Head(vals);
      }

      vars = Tail(vars);
      vals = Tail(vals);
    }
    env = ParentEnv(env);
  }

  Error("Unbound variable \"%s\"", SymbolName(var));
}

void SetVariable(Val var, Val val, Val env)
{
  while (!IsNil(env)) {
    Val frame = FirstFrame(env);
    Val vars = FrameVars(frame);
    Val vals = FrameVals(frame);
    while (!IsNil(vars)) {
      if (Eq(var, Head(vars))) {
        SetHead(vals, val);
        return;
      }

      vars = Tail(vars);
      vals = Tail(vals);
    }
    env = ParentEnv(env);
  }

  Error("Unbound variable");
}

void Define(Val var, Val val, Val env)
{
  Val frame = FirstFrame(env);
  Val vars = FrameVars(frame);
  Val vals = FrameVals(frame);
  while (!IsNil(vars)) {
    if (Eq(var, Head(vars))) {
      SetHead(vals, val);
      return;
    }

    vars = Tail(vars);
    vals = Tail(vals);
  }

  AddBinding(var, val, frame);
}
