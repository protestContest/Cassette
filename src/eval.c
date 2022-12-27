#include "eval.h"
#include "env.h"
#include "proc.h"
#include "list.h"
#include "vm.h"

#define IsSelfEval(val)     (IsNum(val) || IsInt(val))
#define IsVariable(val)     IsSym(val)
#define IsQuoted(vm, val)   IsTaggedList(vm, exp, SymbolFor(vm, "quote", 5))
#define IsApplication(val)  IsPair(val)
#define IsAssign(vm, val)   IsTaggedList(vm, exp, SymbolFor(vm, "set!", 4))
#define IsDefine(vm, val)   IsTaggedList(vm, exp, SymbolFor(vm, "def", 3))
#define IsCond(vm, val)     IsTaggedList(vm, exp, SymbolFor(vm, "cond", 4))
#define IsLambda(vm, val)   IsTaggedList(vm, exp, fn_val)
#define IsBegin(vm, exp)    IsTaggedList(vm, exp, SymbolFor(vm, "do", 2))
#define IsTrue(exp)         (!IsNil(exp) && !Eq(exp, false_val))

#define Operator(vm, exp)   Head(vm, exp)
#define Operands(vm, exp)   Tail(vm, exp)

Val QuoteValue(VM *vm, Val exp);
Val MakeLambda(VM *vm, Val params, Val body);

Val EvalSequence(VM *vm, Val exps, Val env);
Val EvalAssign(VM *vm, Val exp, Val env);
Val EvalDefine(VM *vm, Val exp, Val env);
Val EvalCond(VM *vm, Val exp, Val env);
Val EvalLambda(VM *vm, Val exp, Val env);

Val Eval(VM *vm, Val exp, Val env)
{
  if (IsSelfEval(exp))    Debug("self"); return exp;
  if (IsVariable(exp))    Debug("var"); return Lookup(vm, exp, env);
  if (IsQuoted(vm, exp))  Debug("quote"); return QuoteValue(vm, exp);
  if (IsAssign(vm, exp))  Debug("assign"); return EvalAssign(vm, exp, env);
  if (IsDefine(vm, exp))  Debug("define"); return EvalDefine(vm, exp, env);
  if (IsCond(vm, exp))    Debug("cond"); return EvalCond(vm, exp, env);
  if (IsLambda(vm, exp))  Debug("fn"); return EvalLambda(vm, exp, env);
  if (IsBegin(vm, exp))   Debug("begin"); return EvalSequence(vm, exp, env);

  if (IsApplication(exp)) {
    Debug("apply");
    return Apply(vm, Eval(vm, Operator(vm, exp), env), EvalSequence(vm, Operands(vm, exp), env));
  }

  Error("Unknown expression type");
}

Val Apply(VM *vm, Val proc, Val args)
{
  if (IsPrimitiveProc(vm, proc)) {
    return ApplyPrimitiveProc(vm, proc, args);
  }

  Error("Can't apply");
}

Val QuoteValue(VM *vm, Val exp)
{
  return Head(vm, Tail(vm, exp));
}

Val MakeLambda(VM *vm, Val params, Val body)
{
  return MakePair(vm, SymbolFor(vm, "fn", 2), MakePair(vm, params, body));
}

Val EvalSequence(VM *vm, Val exps, Val env)
{
  if (IsNil(exps)) return nil_val;

  return MakePair(vm, Eval(vm, Head(vm, exps), env),
                      EvalSequence(vm, Tail(vm, exps), env));
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
  Val var = ListAt(vm, exp, 1);
  Val val = ListAt(vm, exp, 2);
  if (IsPair(var)) {
    var = Head(vm, var);
    val = MakeLambda(vm, Tail(vm, var), val);
  }

  Define(vm, var, Eval(vm, val, env), env);

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
  Val params = ListAt(vm, exp, 1);
  Val body = ListAt(vm, exp, 2);

  return MakeProcedure(vm, params, body, env);
}

