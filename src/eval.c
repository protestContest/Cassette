#include "eval.h"
#include "env.h"
#include "mem.h"
#include "proc.h"
#include "printer.h"
#include "primitives.h"
#include "module.h"

bool IsSelfEvaluating(Val exp)
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
  return MakeSymbol("ok", 2);
}

Val EvalDefine(Val exp, Val env)
{
  if (IsSym(Head(exp))) {
    Define(Head(exp), Eval(Head(Tail(exp)), env), env);
    return MakeSymbol("ok", 2);
  } else if (IsPair(Head(exp))) {
    Val pattern = Head(exp);
    Val name = Head(pattern);
    Val params = Tail(pattern);
    Val body = Tail(exp);
    Val proc = MakeProc(name, params, body, env);
    Define(name, proc, env);
    return MakeSymbol("ok", 2);
  }

  Error("Can't define that");
}

Val EvalIf(Val exp, Val env)
{
  Val predicate = Head(exp);
  Val consequent = Head(Tail(exp));
  Val alternative = Head(Tail(Tail(exp)));

  Val test = Eval(predicate, env);
  if (IsNil(test) || Eq(test, MakeSymbol("false", 5))) {
    return Eval(alternative, env);
  } else {
    return Eval(consequent, env);
  }
}

Val EvalLambda(Val exp, Val env)
{
  return MakeProc(MakeSymbol("λ", 2), Head(exp), Tail(exp), env);
}

Val EvalSequence(Val exp, Val env)
{
  if (IsNil(Tail(exp))) {
    return Eval(Head(exp), env);
  } else {
    Eval(Head(exp), env);
    return EvalSequence(Tail(exp), env);
  }
}

Val EvalList(Val exp, Val env)
{
  if (IsNil(exp)) return nil;
  return MakePair(Eval(Head(exp), env), EvalList(Tail(exp), env));
}

Val EvalApplication(Val exp, Val env)
{
  Val values = EvalList(exp, env);
  return Apply(Head(values), Tail(values));
}

Val EvalLookup(Val exp, Val env)
{
  Val var = Eval(Head(exp), env);
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
  path[len] = '.';
  path[len+1] = 'r';
  path[len+2] = 'y';
  path[len+3] = 'e';
  path[len+4] = '\0';

  Val ok = LoadModule(path, BaseEnv(env));
  DumpEnv(env);
  return ok;
}

Val Eval(Val exp, Val env)
{
  if (IsSelfEvaluating(exp))                  return exp;
  if (IsSym(exp))                             return Lookup(exp, env);
  if (IsTagged(exp, MakeSymbol("quote", 5)))  return Tail(exp);
  if (IsTagged(exp, MakeSymbol("set!", 4)))   return EvalAssignment(Tail(exp), env);
  if (IsTagged(exp, MakeSymbol("def", 3)))    return EvalDefine(Tail(exp), env);
  if (IsTagged(exp, MakeSymbol("if", 2)))     return EvalIf(Tail(exp), env);
  if (IsTagged(exp, MakeSymbol("fn", 2)))     return EvalLambda(Tail(exp), env);
  if (IsTagged(exp, MakeSymbol("λ", 2)))      return EvalLambda(Tail(exp), env);
  if (IsTagged(exp, MakeSymbol("do", 2)))     return EvalSequence(Tail(exp), env);
  if (IsTagged(exp, MakeSymbol("load", 4)))   return EvalLoad(Tail(exp), env);
  if (IsTagged(exp, MakeSymbol("lookup", 6))) return EvalLookup(Tail(exp), env);
  if (IsPair(exp))                            return EvalApplication(exp, env);

  Error("Unknown expression: %s", ValStr(exp));
}

Val Apply(Val proc, Val args)
{
  if (IsTagged(proc, MakeSymbol("prim", 4))) {
    return DoPrimitive(Tail(proc), args);
  } else if (IsTagged(proc, MakeSymbol("proc", 4))) {
    Val env = ExtendEnv(ProcParams(proc), args, ProcEnv(proc));
    return EvalSequence(ProcBody(proc), env);
  }

  Error("Unknown procedure type: %s", ValStr(proc));
}
