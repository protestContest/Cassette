#include "env.h"
#include "proc.h"
#include "printer.h"
#include "vm.h"

Val InitialEnv(VM *vm)
{
  Val env = ExtendEnv(vm, nil_val, nil_val, nil_val);
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
  printf("Extend\n");
  PrintEnv(vm, env);
  PrintValue(vm, vars);
  printf("\n");
  Val new_env = MakePair(vm, MakeFrame(vm, vars, vals), env);
  PrintEnv(vm, new_env);
  return new_env;
}

void PrintEnv(VM *vm, Val env)
{
  if (IsNil(env)) {
    return;
  }

  Val vars = FrameVars(vm, CurFrame(vm, env));
  Val vals = FrameVals(vm, CurFrame(vm, env));

  while (!IsNil(vars)) {
    PrintValue(vm, Head(vm, vars));
    printf(": ");
    PrintValue(vm, Head(vm, vals));
    printf("\n");
    vars = Tail(vm, vars);
    vals = Tail(vm, vals);
  }

  printf("---\n");
  PrintEnv(vm, Tail(vm, env));
}

Val Lookup(VM *vm, Val var, Val env)
{
  while (!IsNil(env)) {
    PrintEnv(vm, env);

    Val vars = FrameVars(vm, CurFrame(vm, env));
    Val vals = FrameVals(vm, CurFrame(vm, env));

    assert(IsPair(vars));
    printf("ok\n");

    while (!IsNil(vars)) {
      if (Eq(Head(vm, vars), var)) {
        return Head(vm, vals);
      }

      vars = Tail(vm, vars);
      vals = Tail(vm, vals);
    }

    env = ParentEnv(vm, env);
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
