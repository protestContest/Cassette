#include "eval.h"
#include "env.h"
#include "mem.h"
#include "proc.h"
#include "printer.h"
#include "primitives.h"
#include "module.h"

#define DEBUG_EVAL 0

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
  } else if (IsList(First(exp))) {
    Val pattern = First(exp);
    Val name = Head(pattern);
    Val params = Tail(pattern);
    Val body = Second(exp);
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
  Val alternative = nil;

  if (IsTagged(consequent, "else")) {
    alternative = Third(consequent);
    consequent = Second(consequent);
  }

  Val test = EvalIn(predicate, env);

  if (IsTrue(test)) {
    return EvalIn(consequent, env);
  } else {
    return EvalIn(alternative, env);
  }
}

Val EvalCond(Val clauses, Val env)
{
  while (!IsNil(clauses)) {
    Val clause = Head(clauses);
    Val test = EvalIn(Head(clause), env);
    if (IsTrue(test)) {
      return EvalIn(Tail(clause), env);
    }

    clauses = Tail(clauses);
  }

  return nil;
}

Val EvalLambda(Val exp, Val env)
{
  Val params = Head(exp);
  if (!IsList(params)) {
    params = MakePair(params, nil);
  }
  Val body = Tail(exp);
  Val proc = MakeProc(MakeSymbol("λ"), params, body, env);
  return proc;
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
  if (!IsProc(Head(values))) {
    return Head(values);
  }
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

bool IsApplication(Val exp)
{
  if (!IsList(exp)) return false;

  if (IsTagged(exp, "|>"))      return false;
  if (IsTagged(exp, "quote"))   return false;
  if (IsTagged(exp, "set!"))    return false;
  if (IsTagged(exp, "def"))     return false;
  if (IsTagged(exp, "if"))      return false;
  if (IsTagged(exp, "->"))      return false;
  if (IsTagged(exp, "do"))      return false;
  if (IsTagged(exp, "load"))    return false;
  if (IsTagged(exp, "lookup"))  return false;
  return true;
}

Val EvalCompose(Val exp, Val env)
{
  Val fn = Second(exp);
  if (!IsApplication(fn)) {
    Error("Can't pipe to this: %s", ValStr(fn));
  }

  Val application = EvalList(fn, env);
  Val operand = EvalIn(First(exp), env);
  Val op = Head(application);
  Val args = MakePair(operand, Tail(application));
  return Apply(op, args);
}

Val EvalIn(Val exp, Val env)
{
  if (DEBUG_EVAL) {
    fprintf(stderr, "Evaluating ");
    PrintVal(exp);
  }

  Val result = nil;

  if (IsSelfEvaluating(exp))        result = exp;
  else if (IsSym(exp))              result = Lookup(exp, env);
  else if (IsTagged(exp, "|>"))     result = EvalCompose(Tail(exp), env);
  else if (IsTagged(exp, "quote"))  result = Tail(exp);
  else if (IsTagged(exp, "set!"))   result = EvalAssignment(Tail(exp), env);
  else if (IsTagged(exp, "def"))    result = EvalDefine(Tail(exp), env);
  else if (IsTagged(exp, "if"))     result = EvalIf(Tail(exp), env);
  else if (IsTagged(exp, "cond"))   result = EvalCond(Tail(exp), env);
  else if (IsTagged(exp, "->"))     result = EvalLambda(Tail(exp), env);
  else if (IsTagged(exp, "do"))     result = EvalSequence(Tail(exp), env);
  else if (IsTagged(exp, "load"))   result = EvalLoad(Tail(exp), env);
  else if (IsTagged(exp, "lookup")) result = EvalLookup(Tail(exp), env);
  else if (IsApplication(exp))      result = EvalApplication(exp, env);

  return result;

  Error("Unknown expression: %s", ValStr(exp));
}

Val Apply(Val proc, Val args)
{
  if (DEBUG_EVAL) {
    fprintf(stderr, "Applying ");
    PrintVal(proc);
  }
  if (IsTagged(proc, "prim")) {
    return DoPrimitive(Tail(proc), args);
  } else if (IsTagged(proc, "proc")) {
    Val env = ExtendEnv(ProcParams(proc), args, ProcEnv(proc));
    return EvalIn(ProcBody(proc), env);
  }

  // Error("Unknown procedure type: %s", ValStr(proc));
  return proc;
}

Val Eval(Val exp)
{
  return EvalIn(exp, InitialEnv());
}
