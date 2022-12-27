#include "eval.h"
#include "env.h"
#include "proc.h"

#define IsSelfEval(val)     (IsNum(val) || IsInt(val))
#define IsVariable(val)     IsSym(val)
#define IsQuoted(vm, val)   IsTaggedList(vm, exp, SymbolFor(vm, "quote", 5))
#define IsApplication(val)  IsPair(val)
#define Operator(vm, exp)   Head(vm, exp)
#define Operands(vm, exp)   Tail(vm, exp)

Val QuoteValue(VM *vm, Val exp);
Val EvalList(VM *vm, Val exps, Val env);

Val Eval(VM *vm, Val exp, Val env)
{
  if (IsSelfEval(exp))    return exp;
  if (IsVariable(exp))    return Lookup(vm, exp, env);
  if (IsQuoted(vm, exp))  return QuoteValue(vm, exp);
  if (IsPair(exp)) {
    return Apply(vm, Eval(vm, Operator(vm, exp), env), EvalList(vm, Operands(vm, exp), env));
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

Val EvalList(VM *vm, Val exps, Val env)
{
  if (IsNil(exps)) return nil_val;

  return MakePair(vm, Eval(vm, Head(vm, exps), env),
                      EvalList(vm, Tail(vm, exps), env));
}
