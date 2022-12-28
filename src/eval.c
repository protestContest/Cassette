#include "eval.h"
#include "env.h"
#include "proc.h"
#include "list.h"
#include "vm.h"
#include "printer.h"

#define IsSelfEval(val)     (IsNum(val) || IsInt(val))
#define IsVariable(val)     IsSym(val)
#define IsQuoted(vm, val)   IsTagged(vm, exp, MakeSymbol(vm, "quote", 5))
#define IsApplication(val)  IsPair(val)
#define IsAssign(vm, val)   IsTagged(vm, exp, MakeSymbol(vm, "set!", 4))
#define IsDefine(vm, val)   IsTagged(vm, exp, MakeSymbol(vm, "def", 3))
#define IsCond(vm, val)     IsTagged(vm, exp, MakeSymbol(vm, "cond", 4))
#define IsLambda(vm, val)   IsTagged(vm, exp, MakeSymbol(vm, "lambda", 6))
#define IsBegin(vm, exp)    IsTagged(vm, exp, MakeSymbol(vm, "do", 2))
#define IsTrue(exp)         (!IsNil(exp) && !Eq(exp, false_val))
#define IsLastExp(vm, exp)  IsNil(Tail(vm, exp))

#define Operator(vm, exp)   Head(vm, exp)
#define Operands(vm, exp)   Tail(vm, exp)

Val QuoteValue(VM *vm, Val exp);
Val MakeLambda(VM *vm, Val params, Val body);

Val EvalSequence(VM *vm, Val exps, Val env);
Val ListOfValues(VM *vm, Val exps, Val env);
Val EvalAssign(VM *vm, Val exp, Val env);
Val EvalDefine(VM *vm, Val exp, Val env);
Val EvalCond(VM *vm, Val exp, Val env);
Val EvalLambda(VM *vm, Val exp, Val env);
Val ApplyCompoundProc(VM *vm, Val proc, Val args);

Val Eval(VM *vm, Val exp, Val env)
{
  Debug(EVAL, "eval");
  DebugValue(EVAL, vm, exp);
  PrintEnv(vm, env);

  if (IsSelfEval(exp))    {Debug(EVAL, "self"); return exp;}
  if (IsVariable(exp))    {Debug(EVAL, "var"); return Lookup(vm, exp, env);}
  if (IsQuoted(vm, exp))  {Debug(EVAL, "quote"); return QuoteValue(vm, exp);}
  if (IsAssign(vm, exp))  {Debug(EVAL, "set"); return EvalAssign(vm, exp, env);}
  if (IsDefine(vm, exp))  {Debug(EVAL, "def"); return EvalDefine(vm, Tail(vm, exp), env);}
  if (IsCond(vm, exp))    {Debug(EVAL, "cond"); return EvalCond(vm, exp, env);}
  if (IsLambda(vm, exp))  {Debug(EVAL, "fn"); return EvalLambda(vm, exp, env);}
  if (IsBegin(vm, exp))   {Debug(EVAL, "do"); return EvalSequence(vm, Tail(vm, exp), env);}

  if (IsApplication(exp)) {
    Debug(EVAL, "apply");
    return Apply(vm, Eval(vm, Operator(vm, exp), env), ListOfValues(vm, Operands(vm, exp), env));
  }

  Error("Unknown expression type");
}

Val Apply(VM *vm, Val proc, Val args)
{
  if (IsPrimitiveProc(vm, proc)) {
    return ApplyPrimitiveProc(vm, proc, args);
  }

  if (IsCompoundProc(vm, proc)) {
    return ApplyCompoundProc(vm, proc, args);
  }

  Error("Can't apply");
}

Val ApplyCompoundProc(VM *vm, Val proc, Val args)
{
  Val env = ExtendEnv(vm, ProcEnv(vm, proc), ProcParams(vm, proc), args);
  return EvalSequence(vm, ProcBody(vm, proc), env);
}

Val QuoteValue(VM *vm, Val exp)
{
  return Head(vm, Tail(vm, exp));
}

Val MakeLambda(VM *vm, Val params, Val body)
{
  return MakeTuple(vm, 3, MakeSymbol(vm, "lambda", 6), params, body);
}

Val EvalSequence(VM *vm, Val exps, Val env)
{
  if (IsLastExp(vm, exps)) {
    return Eval(vm, Head(vm, exps), env);
  } else {
    Eval(vm, Head(vm, exps), env);
    return EvalSequence(vm, Tail(vm, exps), env);
  }
}

Val ListOfValues(VM *vm, Val exps, Val env)
{
  if (IsNil(exps)) return nil_val;

  return MakePair(vm, Eval(vm, Head(vm, exps), env),
                      ListOfValues(vm, Tail(vm, exps), env));
}

Val EvalAssign(VM *vm, Val exp, Val env)
{
  Val var = ListAt(vm, exp, 1);
  Val val = Eval(vm, ListAt(vm, exp, 2), env);
  SetVar(vm, var, val, env);
  return ok_val;
}

Val EvalDefine(VM *vm, Val exp, Val env)
{
  Val var = Head(vm, exp);
  Val val = Tail(vm, exp);
  if (IsPair(var)) {
    Val name = Head(vm, var);
    Val params = Tail(vm, var);
    Val body = val;

    val = MakeLambda(vm, params, body);
    Define(vm, name, Eval(vm, val, env), env);
  } else {
    Define(vm, var, Eval(vm, val, env), env);
  }

  return ok_val;
}

Val EvalCond(VM *vm, Val exp, Val env)
{
  Val clauses = Tail(vm, exp);

  while (!IsNil(clauses)) {
    Val clause = Head(vm, clauses);
    Val test = Head(vm, clause);
    Val consequent = Head(vm, Tail(vm, clause));
    if (IsTrue(test)) {
      return Eval(vm, consequent, env);
    }
  }

  Error("Unhandled case");
}

Val EvalLambda(VM *vm, Val exp, Val env)
{
  return MakeProcedure(vm, ProcParams(vm, exp), ProcBody(vm, exp), env);
}

