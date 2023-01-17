#include "eval.h"
#include "printer.h"
#include "env.h"
#include "mem.h"
#include "primitives.h"
#include "proc.h"
#include <stdio.h>

#define DEBUG_EVAL 0

static u32 stack_depth = 0;

EvalResult EvalOk(Val exp)
{
  EvalResult res = { EVAL_OK, exp, NULL };
  return res;
}

EvalResult RuntimeError(char *msg)
{
  EvalResult res = { EVAL_ERROR, nil, msg };
  return res;
}

EvalResult EvalIf(Val exp, Val env);
EvalResult EvalLambda(Val exp, Val env);
EvalResult EvalSequence(Val exp, Val env);
EvalResult EvalPipe(Val exp, Val env);
EvalResult EvalApply(Val exp, Val env);
bool IsSelfEvaluating(Val exp);

EvalResult Eval(Val exp, Val env)
{
  if (DEBUG_EVAL) {
    fprintf(stderr, "[%d] Evaluating %s\n", stack_depth++, ValStr(exp));
    DumpEnv(env);
  }

  EvalResult result;

  if (IsSelfEvaluating(exp)) {
    result = EvalOk(exp);
  } else if (IsSym(exp)) {
    result = Lookup(exp, env);
  } else if (IsTagged(exp, "quote")) {
    result = EvalOk(Tail(exp));
  } else if (IsTagged(exp, "if")) {
    result = EvalIf(Tail(exp), env);
  } else if (IsTagged(exp, "->")) {
    result = EvalLambda(Tail(exp), env);
  } else if (IsTagged(exp, "do")) {
    result = EvalSequence(Tail(exp), env);
  } else if (IsTagged(exp, "|>")) {
    result = EvalPipe(Tail(exp), env);
  } else if (IsTagged(exp, "def")) {
    result = EvalOk(nil);
  } else if (IsList(exp)) {
    result = EvalApply(exp, env);
  } else {
    char *msg = NULL;
    PrintInto(msg, "Unknown expression: %s", ValStr(exp));
    return RuntimeError(msg);
  }

  if (DEBUG_EVAL && result.status == EVAL_OK) {
    fprintf(stderr, "[%d] => %s\n", --stack_depth, ValStr(result.value));
  }

  return result;
}

EvalResult Apply(Val proc, Val args)
{
  if (DEBUG_EVAL) fprintf(stderr, "Applying %s to %s\n", ValStr(proc), ValStr(args));

  if (IsTagged(proc, "primitive")) {
    return DoPrimitive(Tail(proc), args);
  } else if (IsTagged(proc, "procedure")) {
    Val env = ExtendEnv(ProcEnv(proc), ProcParams(proc), args);
    return Eval(ProcBody(proc), env);
  }

  char *msg = NULL;
  PrintInto(msg, "Unknown procedure: %s", ValStr(proc));
  return RuntimeError(msg);
}

EvalResult EvalIf(Val exp, Val env)
{
  Val predicate = First(exp);
  Val consequent = Second(exp);
  Val alternative = Third(exp);
  EvalResult result = Eval(predicate, env);
  if (result.status == EVAL_ERROR) return result;

  if (IsTrue(result.value)) {
    return Eval(consequent, env);
  } else {
    return Eval(alternative, env);
  }
}

EvalResult EvalLambda(Val exp, Val env)
{
  Val params = First(exp);
  Val body = Second(exp);
  return EvalOk(MakeProc(MakeSymbol("λ"), params, body, env));
}

Val DefVariable(Val exp)
{
  if (IsSym(First(exp))) {
    return First(exp);
  } else {
    Val template = First(exp);
    return Head(template);
  }
}

Val DefValue(Val exp)
{
  if (IsSym(First(exp))) {
    return Second(exp);
  } else {
    Val template = First(exp);
    Val params = Tail(template);
    Val body = Second(exp);
    return MakeTagged(3, "->", params, body);
  }
}

EvalResult EvalDefines(Val vars, Val vals, Val env)
{
  if (DEBUG_EVAL) {
    fprintf(stderr, "Evaluating block defines: %s\n", ValStr(vars));
  }

  if (IsNil(vars)) return EvalOk(env);

  Val new_env = AddFrame(env, ListLength(vars));

  while (!IsNil(vars)) {
    if (DEBUG_EVAL) {
      fprintf(stderr, "  %s => %s\n", ValStr(Head(vars)), ValStr(Head(vals)));
    }

    EvalResult result = Eval(Head(vals), new_env);
    if (result.status == EVAL_ERROR) return result;

    DictSet(Head(new_env), Head(vars), result.value);

    vars = Tail(vars);
    vals = Tail(vals);
  }

  return EvalOk(new_env);
}

EvalResult EvalEach(Val exp, Val env)
{
  EvalResult result = Eval(Head(exp), env);
  if (IsNil(Tail(exp)) || result.status == EVAL_ERROR) return result;
  return EvalEach(Tail(exp), env);
}

EvalResult EvalSequence(Val exp, Val env)
{
  Val vars = nil;
  Val vals = nil;
  Val exps = exp;

  while (!IsNil(exps)) {
    Val cur_exp = Head(exps);
    if (IsTagged(cur_exp, "def")) {
      vars = MakePair(DefVariable(Tail(cur_exp)), vars);
      vals = MakePair(DefValue(Tail(cur_exp)), vals);
    }
    exps = Tail(exps);
  }

  EvalResult env_result = EvalDefines(vars, vals, env);
  if (env_result.status == EVAL_ERROR) return env_result;

  return EvalEach(exp, env_result.value);
}

EvalResult EvalPipe(Val exp, Val env)
{
  Val arg = First(exp);
  exp = Second(exp);

  if (IsList(exp)) {
    exp = MakePair(Head(exp), MakePair(arg, Tail(exp)));
  } else {
    exp = MakeList(2, exp, arg);
  }

  return Eval(exp, env);
}

EvalResult EvalApply(Val exp, Val env)
{
  if (DEBUG_EVAL) fprintf(stderr, "Applying %s\n", ValStr(exp));

  EvalResult proc = Eval(Head(exp), env);
  if (proc.status == EVAL_ERROR) return proc;

  Val args = nil;
  exp = Tail(exp);
  while (!IsNil(exp)) {
    EvalResult arg = Eval(Head(exp), env);
    if (arg.status == EVAL_ERROR) return arg;
    args = MakePair(arg.value, args);
    exp = Tail(exp);
  }

  return Apply(proc.value, Reverse(args));
}

bool IsSelfEvaluating(Val exp)
{
  return IsNil(exp) || IsNum(exp) || IsInt(exp) || IsBin(exp) || IsTuple(exp) || IsDict(exp);
}

void PrintEvalError(EvalResult result)
{
  if (result.status != EVAL_ERROR) return;

  fprintf(stderr, "\x1B[31m");
  fprintf(stderr, "Runtime error: %s\n\n", result.error);
  fprintf(stderr, "\x1B[0m");
}
