#include "interpret.h"
#include "mem.h"
#include "primitives.h"

#define DEBUG_EVAL

Val InitialEnv(Mem *mem);
Val Eval(Val exp, Val env, Mem *mem);
Val Apply(Val exp, Val env, Mem *mem);
Val ApplyPrimitive(Val proc, Val args, Mem *mem);
Val ExtendEnv(Val env, Val vars, Val vals, Mem *mem);
Val Lookup(Val exp, Val env, Mem *mem);
Val EvalLambda(Val exp, Val env, Mem *mem);
Val EvalApply(Val exp, Val env, Mem *mem);
Val EvalEach(Val exps, Val env, Mem *mem);
Val EvalBlock(Val exps, Val env, Mem *mem);
Val EvalDefine(Val exps, Val env, Mem *mem);
Val EvalLet(Val exps, Val env, Mem *mem);
Val EvalIf(Val exps, Val env, Mem *mem);
Val EvalCond(Val exps, Val env, Mem *mem);
Val RuntimeError(char *message, Val exp, Mem *mem);
void PrintEnv(Val env, Mem *mem);

Val Interpret(Val ast, Mem *mem)
{
  if (IsNil(ast)) return nil;
  Val env = InitialEnv(mem);
  return Eval(ast, env, mem);
}

Val InitialEnv(Mem *mem)
{
  MakeSymbol(mem, "primitive");
  Val env = MakePair(mem, nil, nil);
  Define(env, MakeSymbol(mem, "nil"), nil, mem);
  Define(env, MakeSymbol(mem, "true"), SymbolFor("true"), mem);
  Define(env, MakeSymbol(mem, "false"), SymbolFor("false"), mem);
  DefinePrimitives(env, mem);
  return env;
}

static u32 indent = 0;
Val Eval(Val exp, Val env, Mem *mem)
{
#ifdef DEBUG_EVAL
  Print("\n");
  for (u32 i = 0; i < indent; i++) Print("  ");
  Print("Eval: ");
  PrintVal(mem, exp);
  // Print("\nEnv:\n");
  // PrintEnv(env, mem);
#endif

  indent++;
  Val result;

  if (IsNumeric(exp) || IsObj(exp)) {
    result = exp;
  } else if (IsSym(exp)) {
    result = Lookup(exp, env, mem);
  } else if (IsTagged(mem, exp, SymbolFor("->"))) {
    result = EvalLambda(exp, env, mem);
  } else if (IsTagged(mem, exp, SymbolFor("do"))) {
    result = EvalBlock(exp, env, mem);
  } else if (IsTagged(mem, exp, SymbolFor("def"))) {
    result = EvalDefine(exp, env, mem);
  } else if (IsTagged(mem, exp, SymbolFor("let"))) {
    result = EvalLet(exp, env, mem);
  } else if (IsTagged(mem, exp, SymbolFor("if"))) {
    result = EvalIf(exp, env, mem);
  } else if (IsTagged(mem, exp, SymbolFor("cond"))) {
    result = EvalCond(exp, env, mem);
  } else {
    result = EvalApply(exp, env, mem);
  }

  indent--;

#ifdef DEBUG_EVAL
  for (u32 i = 0; i < indent; i++) Print("  ");
  Print("=> ");
  PrintVal(mem, result);
  Print("\n");
#endif

  return result;
}

Val Apply(Val proc, Val args, Mem *mem)
{
  if (IsTagged(mem, proc, SymbolFor("primitive"))) {
    return ApplyPrimitive(proc, args, mem);
  }

  if (!IsTagged(mem, proc, SymbolFor("proc"))) {
    return RuntimeError("Not a procedure", proc, mem);
  }

  Val params = ListAt(mem, proc, 1);
  Val body = ListAt(mem, proc, 2);
  Val env = MakePair(mem, nil, ListAt(mem, proc, 3));

  if (IsSym(params)) {
    Define(env, params, args, mem);
  } else {
    while (!IsNil(args)) {
      if (IsNil(params)) {
        return RuntimeError("Too many arguments", args, mem);
      }
      Define(env, Head(mem, params), Head(mem, args), mem);
      params = Tail(mem, params);
      args = Tail(mem, args);
    }

    if (!IsNil(params)) {
      return MakeList(mem, 4, MakeSymbol(mem, "proc"), params, body , env);
    }
  }

  return Eval(body, env, mem);
}

Val ApplyPrimitive(Val proc, Val args, Mem *mem)
{
  Val op = Tail(mem, proc);
  return DoPrimitive(op, args, mem);
}

Val ExtendEnv(Val env, Val vars, Val vals, Mem *mem)
{
  env = MakePair(mem, nil, env);
  while (!IsNil(vars)) {
    Define(env, Head(mem, vars), Head(mem, vals), mem);
    vars = Tail(mem, vars);
    vals = Tail(mem, vals);
  }

  return env;
}

Val Lookup(Val var, Val env, Mem *mem)
{
  while (!IsNil(env)) {
    Val frame = Head(mem, env);

    while (!IsNil(frame)) {
      Val item = Head(mem, frame);

      if (Eq(var, Head(mem, item))) {
        return Tail(mem, item);
      }

      frame = Tail(mem, frame);
    }

    env = Tail(mem, env);
  }

  return RuntimeError("Unbound variable", var, mem);
}

void Define(Val env, Val var, Val value, Mem *mem)
{
  Val frame = Head(mem, env);

  if (IsNil(frame)) {
    Val new_item = MakePair(mem, var, value);
    frame = MakePair(mem, new_item, nil);
    SetHead(mem, env, frame);
    return;
  }

  while (true) {
    Val item = Head(mem, frame);
    if (Eq(Head(mem, item), var)) {
      SetTail(mem, item, value);
      return;
    }

    if (IsNil(Tail(mem, frame))) {
      Val new_item = MakePair(mem, var, value);
      SetTail(mem, frame, MakePair(mem, new_item, nil));
      return;
    }
    frame = Tail(mem, frame);
  }
}

Val EvalLambda(Val exp, Val env, Mem *mem)
{
  Val params = ListAt(mem, exp, 1);
  Val body = ListAt(mem, exp, 2);
  return MakeList(mem, 4, MakeSymbol(mem, "proc"), params, body, env);
}

Val EvalApply(Val exp, Val env, Mem *mem)
{
  Val proc = Eval(Head(mem, exp), env, mem);
  Val args = EvalEach(Tail(mem, exp), env, mem);

  if (IsNumeric(proc) && IsNil(args)) {
    return proc;
  }

  return Apply(proc, args, mem);
}

Val EvalEach(Val exps, Val env, Mem *mem)
{
  Val vals = nil;
  while (!IsNil(exps)) {
    Val val = Eval(Head(mem, exps), env, mem);
    vals = MakePair(mem, val, vals);
    exps = Tail(mem, exps);
  }
  return ReverseOnto(mem, vals, nil);
}

Val EvalBlock(Val exps, Val env, Mem *mem)
{
  Val block = Tail(mem, exps);
  Val result = nil;
  while (!IsNil(block)) {
    result = Eval(Head(mem, block), env, mem);
    block = Tail(mem, block);
  }
  return result;
}

Val EvalDefine(Val exp, Val env, Mem *mem)
{
  Val ids = ListAt(mem, exp, 1);
  Val var = Head(mem, ids);
  Val params = Tail(mem, ids);
  if (!IsSym(var)) return RuntimeError("Define must be an ID", var, mem);
  Val body = ListAt(mem, exp, 2);
  Val val = MakeList(mem, 4, MakeSymbol(mem, "proc"), params, body, env);
  Define(env, var, val, mem);
  return val;
}

Val EvalLet(Val exps, Val env, Mem *mem)
{
  Val pairs = Tail(mem, exps);
  Val val;
  while (!IsNil(pairs)) {
    Val pair = Head(mem, pairs);
    Val var = Head(mem, pair);
    val = Eval(Tail(mem, pair), env, mem);
    Define(env, var, val, mem);
    pairs = Tail(mem, pairs);
  }
  return val;
}

Val EvalIf(Val exps, Val env, Mem *mem)
{
  Val condition = ListAt(mem, exps, 1);
  Val consequent = ListAt(mem, exps, 2);
  Val alternative = ListAt(mem, exps, 3);

  Val test = Eval(condition, env, mem);
  if (IsTrue(test)) {
    return Eval(consequent, env, mem);
  } else {
    return Eval(alternative, env, mem);
  }
}

Val EvalCond(Val exps, Val env, Mem *mem)
{
  Val clauses = Tail(mem, exps);
  while (!IsNil(clauses)) {
    Val clause = Head(mem, clauses);
    Val test = Eval(Head(mem, clause), env, mem);
    if (IsTrue(test)) {
      return Eval(Tail(mem, clause), env, mem);
    }
    clauses = Tail(mem, clauses);
  }

  return RuntimeError("Unmatched cond", clauses, mem);
}

bool IsTrue(Val value)
{
  return !(IsNil(value) || Eq(value, SymbolFor("false")));
}

Val RuntimeError(char *message, Val exp, Mem *mem)
{
  Print(message);
  Print(": ");
  PrintVal(mem, exp);
  Print("\n");
  Exit();
  return nil;
}

void PrintEnv(Val env, Mem *mem)
{
  if (IsNil(env)) Print("<empty env>");
  while (!IsNil(env)) {
    Print("----------------\n");
    Val frame = Head(mem, env);

    while (!IsNil(frame)) {
      Val item = Head(mem, frame);

      Val var = Head(mem, item);
      Val val = Tail(mem, item);
      Print(SymbolName(mem, var));
      Print(": ");
      PrintVal(mem, val);
      Print("\n");

      frame = Tail(mem, frame);
    }

    env = Tail(mem, env);
  }
}
