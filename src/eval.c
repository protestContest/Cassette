#include "eval.h"
#include "env.h"
#include "mem.h"
#include "proc.h"
#include "printer.h"

Val EvalTuple(Val exp, Val env)
{

  u32 size = RawVal(TupleLength(exp));
  Val tuple = MakeTuple(size);
  for (u32 i = 0; i < size; i++) {
    TupleSet(tuple, i, Eval(TupleAt(exp, i), env));
  }
  return tuple;
}

Val EvalPair(Val exp, Val env)
{

  return MakePair(Eval(Head(exp), env), Eval(Tail(exp), env));
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
    Define(Head(exp), Head(Tail(exp)), env);
    return MakeSymbol("ok", 2);
  } else if (IsPair(Head(exp))) {
    Val pattern = Head(exp);
    Val var = Head(pattern);
    Val params = Tail(pattern);
    Val body = Tail(exp);
    Val proc = MakeProc(params, body, env);
    Define(var, proc, env);
    return MakeSymbol("ok", 2);
  }

  Error("Can't define that");
}

Val EvalCond(Val exp, Val env)
{

  if (IsNil(exp)) {
    Error("Unhandled condition");
  }

  Val clause = Head(exp);
  Val predicate = Head(clause);
  Val consequent = Head(Tail(clause));

  if (Eq(predicate, MakeSymbol("else", 4))) {
    return Eval(consequent, env);
  }

  Val pass = Eval(predicate, env);
  if (!IsNil(pass) && !Eq(pass, MakeSymbol("false", 5))) {
    return Eval(consequent, env);
  }

  return EvalCond(Tail(exp), env);
}

Val EvalLambda(Val exp, Val env)
{

  return MakeProc(Head(exp), Tail(exp), env);
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

Val Eval(Val exp, Val env)
{
  if (IsNum(exp))                             return exp;
  if (IsInt(exp))                             return exp;
  if (IsTuple(exp))                           return EvalTuple(exp, env);
  if (IsSym(exp))                             return Lookup(exp, env);
  if (IsTagged(exp, MakeSymbol("quote", 5)))  return Tail(exp);
  if (IsTagged(exp, MakeSymbol("set!", 4)))   return EvalAssignment(Tail(exp), env);
  if (IsTagged(exp, MakeSymbol("def", 3)))    return EvalDefine(Tail(exp), env);
  if (IsTagged(exp, MakeSymbol("cond", 4)))   return EvalCond(Tail(exp), env);
  if (IsTagged(exp, MakeSymbol("fn", 2)))     return EvalLambda(Tail(exp), env);
  if (IsTagged(exp, MakeSymbol("do", 2)))     return EvalSequence(Tail(exp), env);
  if (IsPair(exp) && !IsPair(Tail(exp)))      return EvalPair(exp, env);
  if (IsPair(exp))                            return EvalApplication(exp, env);

  Error("Unknown expression");
}

Val Apply(Val proc, Val args)
{
  if (IsTagged(proc, MakeSymbol("prim", 4))) {
    return DoPrimitive(Tail(proc), args);
  } else if (IsTagged(proc, MakeSymbol("proc", 4))) {
    Val env = ExtendEnv(ProcParams(proc), args, ProcEnv(proc));
    return EvalSequence(ProcBody(proc), env);
  }

  Error("Unknown procedure type");
}
