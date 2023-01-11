#include "eval.h"
#include "env.h"
#include "mem.h"
#include "proc.h"
#include "printer.h"
#include "primitives.h"
#include "module.h"

static bool IsBool(Val exp)
{
  return Eq(exp, SymbolFor("true")) || Eq(exp, SymbolFor("false"));
}

static bool IsSelfEvaluating(Val exp)
{
  return IsNil(exp)
      || IsNum(exp)
      || IsInt(exp)
      || IsBin(exp)
      || IsTuple(exp)
      || IsDict(exp);
}

Val EvalAssignment(Val exp, Val env)
{
  Val var = Head(exp);
  Val val = Head(Tail(exp));
  SetVariable(var, val, env);
  return nil;
}

Val EvalDefine(Val exp, Val env)
{
  if (IsSym(First(exp))) {
    Define(First(exp), EvalIn(Second(exp), env), env);
    return nil;
  } else if (IsPair(Head(exp))) {
    Val pattern = Head(exp);
    Val name = Head(pattern);
    Val params = Tail(pattern);
    Val body = Tail(exp);
    Val proc = MakeProc(name, params, body, env);
    Define(name, proc, env);
    return nil;
  }

  Error("Can't define that");
}

Val EvalIf(Val exp, Val env)
{
  Val predicate = First(exp);
  Val consequent = Second(exp);
  Val alternative = Third(exp);

  Val test = EvalIn(predicate, env);

  if (IsTrue(test)) {
    return EvalIn(consequent, env);
  } else {
    return EvalIn(alternative, env);
  }
}

Val EvalLambda(Val exp, Val env)
{
  return MakeProc(MakeSymbol("λ"), Head(exp), Tail(exp), env);
}

Val EvalSequence(Val exp, Val env)
{
  if (IsNil(Tail(exp))) {
    return EvalIn(Head(exp), env);
  } else {
    EvalIn(Head(exp), env);
    return EvalSequence(Tail(exp), env);
  }
}

Val EvalList(Val exp, Val env)
{
  if (IsNil(exp)) return nil;
  Val head = EvalIn(Head(exp), env);
  Val tail = EvalList(Tail(exp), env);
  return MakePair(head, tail);
}

Val EvalApplication(Val exp, Val env)
{
  Val values = EvalList(exp, env);
  return Apply(Head(values), Tail(values));
}

Val EvalLookup(Val exp, Val env)
{
  Val var = EvalIn(Head(exp), env);
  return Lookup(var, env);
}

Val EvalLoad(Val exp, Val env)
{
  Val filename = Head(exp);
  if (!IsBin(filename)) {
    Error("Can't load \"%s\"", ValStr(filename));
  }

  u32 len = BinaryLength(filename);
  char path[len+5];
  for (u32 i = 0; i < len; i++) {
    path[i] = BinaryData(filename)[i];
  }
  strcpy(&path[len], ".rye");

  return LoadModule(path, GlobalEnv(env));
}

Val EvalIn(Val exp, Val env)
{
  printf("Evaluating ");
  PrintVal(exp);

  if (IsSelfEvaluating(exp))   return exp;
  if (IsSym(exp))              return Lookup(exp, env);
  if (IsTagged(exp, "quote"))  return Tail(exp);
  if (IsTagged(exp, "set!"))   return EvalAssignment(Tail(exp), env);
  if (IsTagged(exp, "def"))    return EvalDefine(Tail(exp), env);
  if (IsTagged(exp, "if"))     return EvalIf(Tail(exp), env);
  if (IsTagged(exp, "fn"))     return EvalLambda(Tail(exp), env);
  if (IsTagged(exp, "λ"))      return EvalLambda(Tail(exp), env);
  if (IsTagged(exp, "do"))     return EvalSequence(Tail(exp), env);
  if (IsTagged(exp, "load"))   return EvalLoad(Tail(exp), env);
  if (IsTagged(exp, "lookup")) return EvalLookup(Tail(exp), env);
  if (IsPair(exp))             return EvalApplication(exp, env);

  Error("Unknown expression: %s", ValStr(exp));
}

Val Apply(Val proc, Val args)
{
  if (IsTagged(proc, "prim")) {
    return DoPrimitive(Tail(proc), args);
  } else if (IsTagged(proc, "proc")) {
    Val env = ExtendEnv(ProcParams(proc), args, ProcEnv(proc));
    return EvalSequence(ProcBody(proc), env);
  }

  Error("Unknown procedure type: %s", ValStr(proc));
}

Val Eval(Val exp)
{
  return EvalIn(exp, InitialEnv());
}
