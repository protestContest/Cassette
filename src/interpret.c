#include "interpret.h"
#include "mem.h"
#include "primitives.h"
#include "env.h"

Val ApplyPrimitive(Val proc, Val args, Mem *mem);
Val EvalAccess(Val exp, Val env, Mem *mem);
Val EvalLambda(Val exp, Val env, Mem *mem);
Val EvalApply(Val exp, Val env, Mem *mem);
Val EvalEach(Val exps, Val env, Mem *mem);
Val EvalBlock(Val exps, Val env, Mem *mem);
Val EvalDefine(Val exps, Val env, Mem *mem);
Val EvalLet(Val exps, Val env, Mem *mem);
Val EvalIf(Val exps, Val env, Mem *mem);
Val EvalCond(Val exps, Val env, Mem *mem);
Val EvalImport(Val exps, Val env, Mem *mem);
Val RuntimeError(char *message, Val exp, Mem *mem);
void PrintEnv(Val env, Mem *mem);

Val RunFile(char *filename, Mem *mem)
{
  char *src;
  src = ReadFile(filename);
  if (!src) {
    Print("Could not open file");
    Exit();
  }

  Val ast = Parse(src, mem);
  return Interpret(ast, mem);
}

Val Interpret(Val ast, Mem *mem)
{
  if (IsNil(ast)) return nil;
  Val env = InitialEnv(mem);
  return Eval(ast, env, mem);
}

#ifdef DEBUG_EVAL
static u32 indent = 0;
#endif

Val Eval(Val exp, Val env, Mem *mem)
{
#ifdef DEBUG_EVAL
  Print("\n");
  for (u32 i = 0; i < indent; i++) Print("  ");
  Print("Eval: ");
  PrintVal(mem, exp);
  // Print("\nEnv:\n");
  // PrintEnv(env, mem);
  indent++;
#endif

  Val result;

  if (IsNil(exp) || IsNumeric(exp) || IsObj(exp)) {
    result = exp;
  } else if (IsSym(exp)) {
    result = Lookup(exp, env, mem);
  } else if (IsTagged(mem, exp, SymbolFor("->"))) {
    result = EvalLambda(exp, env, mem);
  } else if (IsTagged(mem, exp, SymbolFor(":"))) {
    result = ListAt(mem, exp, 1);
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
  } else if (IsTagged(mem, exp, SymbolFor("."))) {
    result = EvalAccess(exp, env, mem);
  } else if (IsTagged(mem, exp, SymbolFor("import"))) {
    result = EvalImport(exp, env, mem);
  } else {
    result = EvalApply(exp, env, mem);
  }


#ifdef DEBUG_EVAL
  indent--;
  for (u32 i = 0; i < indent; i++) Print("  ");
  Print("=> ");
  PrintVal(mem, result);
  Print("\n");
#endif

  return result;
}

Val ListAccess(Val list, Val index, Mem *mem)
{
  if (!IsInt(index)) {
    return RuntimeError("Must access lists with integers", index, mem);
  }

  return ListAt(mem, list, RawInt(index));
}

Val Apply(Val proc, Val args, Mem *mem)
{
  if (IsTagged(mem, proc, SymbolFor("α"))) {
    return ApplyPrimitive(proc, args, mem);
  }

  if (!IsTagged(mem, proc, SymbolFor("λ"))) {
    if (ListLength(mem, args) == 1) return ListAccess(proc, Head(mem, args), mem);
    return RuntimeError("Not a procedure", proc, mem);
  }

#ifdef DEBUG_EVAL
  Print("Apply ");
  PrintVal(mem, proc);
  Print(" ");
  PrintVal(mem, args);
  Print("\n");
#endif

  Val params = ListAt(mem, proc, 1);
  Val body = ListAt(mem, proc, 2);
  Val env = ExtendEnv(ListAt(mem, proc, 3), mem);

  if (IsSym(params)) {
    Define(params, args, env, mem);
  } else {
    while (!IsNil(args)) {
      if (IsNil(params)) {
        return RuntimeError("Too many arguments", args, mem);
      }
      Define(Head(mem, params), Head(mem, args), env, mem);
      params = Tail(mem, params);
      args = Tail(mem, args);
    }

    if (!IsNil(params)) {
      return MakeProcedure(params, body , env, mem);
    }
  }

  return Eval(body, env, mem);
}

Val ApplyPrimitive(Val proc, Val args, Mem *mem)
{
  Val op = Tail(mem, proc);
  return DoPrimitive(op, args, mem);
}

Val EvalAccess(Val exp, Val env, Mem *mem)
{
  Val dict = Eval(ListAt(mem, exp, 1), env, mem);
  Val key = ListAt(mem, exp, 2);
  if (!IsDict(mem, dict)) {
    return RuntimeError("Can't access this", dict, mem);
  }

  return DictGet(mem, dict, key);
}

Val EvalLambda(Val exp, Val env, Mem *mem)
{
  Val params = ListAt(mem, exp, 1);
  Val body = ListAt(mem, exp, 2);
  return MakeList(mem, 4, MakeSymbol(mem, "λ"), params, body, env);
}

Val EvalApply(Val exp, Val env, Mem *mem)
{
  Val proc = Eval(Head(mem, exp), env, mem);
  Val args = EvalEach(Tail(mem, exp), env, mem);

  if (IsNumeric(proc) && IsNil(args)) {
    return proc;
  }

  if (IsDict(mem, proc) && ListLength(mem, args) == 1) {
    return DictGet(mem, proc, Head(mem, args));
  }

  if (IsTuple(mem, proc) && ListLength(mem, args) == 1) {
    Val index = Head(mem, args);
    if (!IsInt(index)) {
      return RuntimeError("Must access tuples with integers", index, mem);
    }
    return TupleAt(mem, proc, RawInt(index));
  }

  if (IsBinary(mem, proc) && ListLength(mem, args) == 1) {
    PrintVal(mem, proc);
    Val index = Head(mem, args);
    if (!IsInt(index)) {
      return RuntimeError("Must access binaries with integers", index, mem);
    }
    return IntVal(BinaryData(mem, proc)[RawInt(index)]);
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
  Val var = ListAt(mem, exp, 1);
  Val val = Eval(ListAt(mem, exp, 2), env, mem);
  Define(var, val, env, mem);
  return SymbolFor("ok");
}

Val EvalLet(Val exps, Val env, Mem *mem)
{
  Val pairs = Tail(mem, exps);
  Val val;
  while (!IsNil(pairs)) {
    Val var = Head(mem, pairs);
    pairs = Tail(mem, pairs);
    val = Eval(Head(mem, pairs), env, mem);
    pairs = Tail(mem, pairs);
    Define(var, val, env, mem);
  }
  return SymbolFor("ok");
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

Val EvalImport(Val exps, Val env, Mem *mem)
{
  Val file_arg = ListAt(mem, exps, 1);
  Val name_arg = ListAt(mem, exps, 2);

  char *filename = (char*)BinaryData(mem, file_arg);
  char *src = ReadFile(filename);
  if (!filename) {
    return RuntimeError("Could not open", file_arg, mem);
  }
  Val ast = Parse(src, mem);
  Val import_env = ExtendEnv(TopEnv(mem, env), mem);

  Val stmts = Tail(mem, ast);
  while (!IsNil(stmts)) {
    Val stmt = Head(mem, stmts);
    if (IsTagged(mem, stmt, SymbolFor("def"))) {
      EvalDefine(stmt, import_env, mem);
    }
    stmts = Tail(mem, stmts);
  }

  Val frame = Head(mem, import_env);
  if (IsNil(name_arg)) {
    Val keys = DictKeys(mem, frame);
    Val vals = DictValues(mem, frame);
    for (u32 i = 0; i < DictSize(mem, frame); i++) {
      Define(TupleAt(mem, keys, i), TupleAt(mem, vals, i), env, mem);
    }
  } else {
    Define(name_arg, frame, env, mem);
  }

  return SymbolFor("ok");
}

Val RuntimeError(char *message, Val exp, Mem *mem)
{
  Print(message);
  Print(": ");
  PrintVal(mem, exp);
  Print("\n");
  return nil;
}
