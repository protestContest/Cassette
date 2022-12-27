#include "env.h"
#include "proc.h"

Val InitialEnv(VM *vm)
{
  Val env = MakePair(vm, MakePair(vm, nil_val, nil_val), nil_val);
  DefinePrimitives(vm, env);
  return env;
}

void AddBinding(VM *vm, Val frame, Val var, Val val)
{
  SetHead(vm, frame, MakePair(vm, var, Head(vm, frame)));
  SetTail(vm, frame, MakePair(vm, val, Tail(vm, frame)));
}

Val ExtendEnv(VM *vm, Val env, Val vars, Val vals)
{
  return MakePair(vm, MakeFrame(vm, vars, vals), env);
}

Val Lookup(VM *vm, Val var, Val env)
{
  while (!IsNil(env)) {
    Val vars = FrameVars(vm, CurFrame(vm, env));
    Val vals = FrameVals(vm, CurFrame(vm, env));

    while (!IsNil(vars)) {
      if (Eq(Head(vm, vars), var)) {
        return Head(vm, vals);
      }

      vars = Tail(vm, vars);
      vals = Tail(vm, vals);
    }

    env = Tail(vm, env);
  }

  Error("Unbound variable \"%s\"", SymbolText(vm, var));
}

void SetVar(VM *vm, Val var, Val val, Val env)
{
  while (!IsNil(env)) {
    Val vars = FrameVars(vm, CurFrame(vm, env));
    Val vals = FrameVals(vm, CurFrame(vm, env));

    while (!IsNil(vars)) {
      if (Eq(Head(vm, vars), var)) {
        SetHead(vm, vals, val);
        return;
      }

      vars = Tail(vm, vars);
      vals = Tail(vm, vals);
    }

    env = Tail(vm, env);
  }

  Error("Unbound variable");
}

void Define(VM *vm, Val var, Val val, Val env)
{
  Val vars = FrameVars(vm, CurFrame(vm, env));
  Val vals = FrameVals(vm, CurFrame(vm, env));

  while (!IsNil(vars)) {
    if (Eq(Head(vm, vars), var)) {
      SetHead(vm, vals, val);
      return;
    }

    vars = Tail(vm, vars);
    vals = Tail(vm, vals);
  }

  AddBinding(vm, CurFrame(vm, env), var, val);
}
