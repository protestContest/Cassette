#include "eval.h"
#include "printer.h"
#include "env.h"
#include "mem.h"
#include "primitives.h"
#include "proc.h"
#include <stdio.h>

static u32 stack_depth = 0;

EvalResult EvalOk(Val exp)
{
  EvalResult res;
  res.status = EVAL_OK;
  res.value = exp;
  return res;
}

EvalResult RuntimeError(char *msg)
{
  EvalResult res;
  res.status = EVAL_ERROR;
  res.error = msg;
  return res;
}

EvalResult EvalIf(Val exp, Val env);
EvalResult EvalCond(Val exp, Val env);
EvalResult EvalLambda(Val exp, Val env);
EvalResult EvalSequence(Val exp, Val env);
EvalResult EvalDefine(Val exp, Val env);
EvalResult EvalPipe(Val exp, Val env);
EvalResult EvalApply(Val exp, Val env);
EvalResult EvalAccess(Val obj, Val key);
EvalResult EvalModule(Val exp, Val env);
EvalResult EvalUse(Val exp, Val env);
EvalResult EvalImport(Val exp, Val env);

EvalResult Eval(Val exp, Val env)
{
  if (DEBUG_EVAL) {
    fprintf(stderr, "[%d] Evaluating %s\n", stack_depth++, ValStr(exp));
    DumpEnv(env, false);
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
  } else if (IsTagged(exp, "cond")) {
    result = EvalCond(Tail(exp), env);
  } else if (IsTagged(exp, "->")) {
    result = EvalLambda(Tail(exp), env);
  } else if (IsTagged(exp, "do")) {
    result = EvalSequence(Tail(exp), env);
  } else if (IsTagged(exp, "|>")) {
    result = EvalPipe(Tail(exp), env);
  } else if (IsTagged(exp, "def")) {
    result = EvalDefine(Tail(exp), env);
  } else if (IsTagged(exp, "module")) {
    result = EvalModule(Tail(exp), env);
  } else if (IsTagged(exp, "use")) {
    result = EvalUse(Tail(exp), env);
  } else if (IsTagged(exp, "import")) {
    result = EvalImport(Tail(exp), env);
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
  } else if (IsAccessable(proc)) {
    return EvalAccess(proc, Head(args));
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

EvalResult EvalCond(Val exp, Val env)
{
  if (IsNil(exp)) return EvalOk(nil);

  Val clause = Head(exp);
  Val pred = Head(clause);
  Val consequent = Tail(clause);
  EvalResult pred_result = Eval(pred, env);
  if (pred_result.status != EVAL_OK) return pred_result;
  if (IsTrue(pred_result.value)) {
    return Eval(consequent, env);
  }

  return EvalCond(Tail(exp), env);
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

EvalResult EvalDefine(Val exp, Val env)
{
  Val frame = Head(env);
  Val var = First(exp);

  if (IsSym(var)) {
    EvalResult val = Eval(Second(exp), env);
    if (val.status != EVAL_OK) return val;
    DictSet(frame, var, val.value);
    return EvalOk(nil);
  } else if (IsList(var)) {
    Val name = Head(var);
    Val params = Tail(var);
    Val body = Second(exp);
    Val proc = MakeProc(name, params, body, env);
    DictSet(frame, name, proc);
    return EvalOk(nil);
  } else {
    char *msg = NULL;
    PrintInto(msg, "Can't define this: \"%s\"", ValStr(var));
    return RuntimeError(msg);
  }
}

EvalResult EvalEach(Val exp, Val env)
{
  EvalResult result = Eval(Head(exp), env);
  if (IsNil(Tail(exp)) || result.status == EVAL_ERROR) return result;
  return EvalEach(Tail(exp), env);
}

EvalResult EvalSequence(Val exp, Val env)
{
  return EvalEach(exp, ExtendEnv(env, nil, nil));
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

bool IsAccessable(Val exp)
{
  return IsDict(exp) || IsTuple(exp) || IsBin(exp) || IsList(exp);
}

EvalResult EvalAccess(Val obj, Val key)
{
  char *err = NULL;
  if (IsDict(obj)) {
    if (!IsSym(key) && !IsBin(key)) {
      PrintInto(err, "Dict keys must be symbols or strings: %s", ValStr(key));
      return RuntimeError(err);
    }

    Val value = DictGet(obj, key);
    return EvalOk(value);
  }

  if (IsTuple(obj)) {
    if (!IsInt(key)) {
      PrintInto(err, "Tuple indexes must be integers: %s", ValStr(key));
      return RuntimeError(err);
    }

    return EvalOk(TupleAt(obj, RawVal(key)));
  }

  if (IsList(obj)) {
    if (!IsInt(key)) {
      PrintInto(err, "List indexes must be integers: %s", ValStr(key));
      return RuntimeError(err);
    }

    return EvalOk(ListAt(obj, RawVal(key)));
  }

  if (IsBin(obj)) {
    if (!IsInt(key)) {
      PrintInto(err, "Binary indexes must be integers: %s", ValStr(key));
      return RuntimeError(err);
    }

    return EvalOk(BinaryAt(obj, RawVal(key)));
  }

  PrintInto(err, "Can't access this: %s", ValStr(obj));
  return RuntimeError(err);
}

EvalResult EvalModuleBody(Val body, Val env)
{
  if (!IsTagged(body, "do")) {
    return RuntimeError("Bad module definition");
  }

  Val mod_env = ExtendEnv(env, nil, nil);
  EvalResult result = EvalEach(Tail(body), mod_env);
  if (result.status != EVAL_OK) return result;

  return EvalOk(Head(mod_env));
}

Val UseProc(Val mod_name, Val env)
{
  Val params = MakePair(MakeSymbol("var"), nil);
  Val get_mod = MakeList(2, MakeSymbol("MODULES"), MakeQuoted(mod_name));
  Val get_var = MakeList(2, get_mod, MakeSymbol("var"));
  Val proc = MakeProc(mod_name, params, get_var, env);
  return proc;
}

EvalResult EvalModule(Val exp, Val env)
{
  Val name = First(exp);

  EvalResult mod_frame = EvalModuleBody(Second(exp), env);
  if (mod_frame.status != EVAL_OK) return mod_frame;

  DefineModule(name, mod_frame.value, env);
  Define(name, UseProc(name, env), env);

  return EvalUse(MakePair(name, nil), env);
}

EvalResult EvalUse(Val exp, Val env)
{
  Val mod_name = First(exp);

  Val proc = UseProc(mod_name, env);
  Define(mod_name, proc, env);

  return EvalOk(MakeSymbol("ok"));
}

EvalResult EvalImport(Val exp, Val env)
{
  char *msg = NULL;
  Val mod_name = First(exp);

  if (!IsSym(mod_name)) {
    PrintInto(msg, "Bad module name: %s", ValStr(mod_name));
    return RuntimeError(msg);
  }

  EvalResult modules = Lookup(MakeSymbol("MODULES"), GlobalEnv(env));
  if (modules.status != EVAL_OK) return modules;

  if (!DictHasKey(modules.value, mod_name)) {
    PrintInto(msg, "Module not loaded: %s", ValStr(mod_name));
    return RuntimeError(msg);
  }

  Val module = DictGet(modules.value, mod_name);
  DictMerge(module, Head(env));
  return EvalOk(MakeSymbol("ok"));
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

  args = Reverse(args);
  return Apply(proc.value, args);
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
