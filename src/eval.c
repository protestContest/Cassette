#include "eval.h"
#include "mem.h"
#include "primitives.h"
#include "env.h"

Val ApplyPrimitive(Val proc, Val args, VM *vm);
Val EvalAccess(Val exp, VM *vm);
Val EvalLambda(Val exp, VM *vm);
Val EvalApply(Val exp, VM *vm);
Val EvalEach(Val exps, VM *vm);
Val EvalBlock(Val exps, VM *vm);
Val EvalDefine(Val exps, VM *vm);
Val EvalLet(Val exps, VM *vm);
Val EvalIf(Val exps, VM *vm);
Val EvalCond(Val exps, VM *vm);
Val EvalAnd(Val exps, VM *vm);
Val EvalOr(Val exps, VM *vm);
Val EvalImport(Val exps, VM *vm);
Val RuntimeError(char *message, Val exp, Mem *mem);
void PrintEnv(Val env, Mem *mem);

Val RunFile(char *filename, Mem *mem)
{
  char *src = (char*)ReadFile(filename);

  if (!src) {
    Print("Could not open file");
    Exit();
  }

  Val ast = Parse(src, StrLen(src), mem);
  VM vm;
  InitVM(&vm, mem);
  return Interpret(ast, &vm);
}

Val Interpret(Val ast, VM *vm)
{
  if (IsNil(ast)) return nil;
  vm->env = InitialEnv(vm->mem);
  return Eval(ast, vm);
}

#ifdef DEBUG_EVAL
static u32 indent = 0;
#endif

Val Eval(Val exp, VM *vm)
{
#ifdef DEBUG_EVAL
  Print("\n");
  for (u32 i = 0; i < indent; i++) Print("  ");
  Print("Eval: ");
  PrintVal(vm->mem, exp);
  // Print("\nEnv:\n");
  // PrintEnv(env, mem);
  indent++;
#endif

  Val result;

  if (IsNil(exp) || IsNumeric(exp) || IsObj(exp)) {
    result = exp;
  } else if (IsSym(exp)) {
    result = Lookup(exp, vm->env, vm->mem);
  } else if (IsTagged(vm->mem, exp, SymbolFor("->"))) {
    result = EvalLambda(exp, vm);
  } else if (IsTagged(vm->mem, exp, SymbolFor(":"))) {
    result = ListAt(vm->mem, exp, 1);
  } else if (IsTagged(vm->mem, exp, SymbolFor("do"))) {
    result = EvalBlock(exp, vm);
  } else if (IsTagged(vm->mem, exp, SymbolFor("def"))) {
    result = EvalDefine(exp, vm);
  } else if (IsTagged(vm->mem, exp, SymbolFor("let"))) {
    result = EvalLet(exp, vm);
  } else if (IsTagged(vm->mem, exp, SymbolFor("if"))) {
    result = EvalIf(exp, vm);
  } else if (IsTagged(vm->mem, exp, SymbolFor("cond"))) {
    result = EvalCond(exp, vm);
  } else if (IsTagged(vm->mem, exp, SymbolFor("and"))) {
    result = EvalAnd(exp, vm);
  } else if (IsTagged(vm->mem, exp, SymbolFor("or"))) {
    result = EvalOr(exp, vm);
  } else if (IsTagged(vm->mem, exp, SymbolFor("."))) {
    result = EvalAccess(exp, vm);
  } else if (IsTagged(vm->mem, exp, SymbolFor("import"))) {
    result = EvalImport(exp, vm);
  } else {
    result = EvalApply(exp, vm);
  }


#ifdef DEBUG_EVAL
  indent--;
  for (u32 i = 0; i < indent; i++) Print("  ");
  Print("=> ");
  PrintVal(vm->mem, result);
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

Val Apply(Val proc, Val args, VM *vm)
{
  if (IsTagged(vm->mem, proc, SymbolFor("α"))) {
    return ApplyPrimitive(proc, args, vm);
  }

  if (!IsTagged(vm->mem, proc, SymbolFor("λ"))) {
    if (ListLength(vm->mem, args) == 1) return ListAccess(proc, Head(vm->mem, args), vm->mem);
    if (ListLength(vm->mem, args) == 0) return proc;
    return RuntimeError("Not a procedure", proc, vm->mem);
  }

#ifdef DEBUG_EVAL
  Print("Apply ");
  PrintVal(vm->mem, proc);
  Print(" ");
  PrintVal(vm->mem, args);
  Print("\n");
#endif

  Val params = ListAt(vm->mem, proc, 1);
  Val body = ListAt(vm->mem, proc, 2);
  Val env = ExtendEnv(ListAt(vm->mem, proc, 3), vm->mem);

  if (IsSym(params)) {
    Define(params, args, env, vm->mem);
  } else {
    while (!IsNil(args)) {
      if (IsNil(params)) {
        return RuntimeError("Too many arguments", args, vm->mem);
      }
      Define(Head(vm->mem, params), Head(vm->mem, args), env, vm->mem);
      params = Tail(vm->mem, params);
      args = Tail(vm->mem, args);
    }

    if (!IsNil(params)) {
      return MakeProcedure(params, body , env, vm->mem);
    }
  }

  VecPush(vm->stack, vm->env);
  vm->env = env;

  Val result = Eval(body, vm);

  vm->env = VecPop(vm->stack, nil);

  return result;
}

Val ApplyPrimitive(Val proc, Val args, VM *vm)
{
  Val op = Tail(vm->mem, proc);
  return DoPrimitive(op, args, vm);
}

Val EvalAccess(Val exp, VM *vm)
{
  Val dict = Eval(ListAt(vm->mem, exp, 1), vm);
  Val key = ListAt(vm->mem, exp, 2);
  if (!IsDict(vm->mem, dict)) {
    return RuntimeError("Can't access this", dict, vm->mem);
  }

  return DictGet(vm->mem, dict, key);
}

Val EvalLambda(Val exp, VM *vm)
{
  Val params = ListAt(vm->mem, exp, 1);
  Val body = ListAt(vm->mem, exp, 2);
  return MakeList(vm->mem, 4, MakeSymbol(vm->mem, "λ"), params, body, vm->env);
}

Val EvalApply(Val exp, VM *vm)
{
  Val proc = Eval(Head(vm->mem, exp), vm);
  Val args = EvalEach(Tail(vm->mem, exp), vm);

  if (IsNumeric(proc) && IsNil(args)) {
    return proc;
  }

  if (IsDict(vm->mem, proc) && ListLength(vm->mem, args) == 1) {
    return DictGet(vm->mem, proc, Head(vm->mem, args));
  }

  if (IsTuple(vm->mem, proc) && ListLength(vm->mem, args) == 1) {
    Val index = Head(vm->mem, args);
    if (!IsInt(index)) {
      return RuntimeError("Must access tuples with integers", index, vm->mem);
    }
    return TupleAt(vm->mem, proc, RawInt(index));
  }

  if (IsBinary(vm->mem, proc) && ListLength(vm->mem, args) == 1) {
    PrintVal(vm->mem, proc);
    Val index = Head(vm->mem, args);
    if (!IsInt(index)) {
      return RuntimeError("Must access binaries with integers", index, vm->mem);
    }
    return IntVal(BinaryData(vm->mem, proc)[RawInt(index)]);
  }

  return Apply(proc, args, vm);
}

Val EvalEach(Val exps, VM *vm)
{
  Val vals = nil;
  while (!IsNil(exps)) {
    Val val = Eval(Head(vm->mem, exps), vm);
    vals = MakePair(vm->mem, val, vals);
    exps = Tail(vm->mem, exps);
  }
  return ReverseOnto(vm->mem, vals, nil);
}

Val EvalBlock(Val exps, VM *vm)
{
  Val block = Tail(vm->mem, exps);
  Val result = nil;
  while (!IsNil(block)) {
    result = Eval(Head(vm->mem, block), vm);
    block = Tail(vm->mem, block);
  }
  return result;
}

Val EvalDefine(Val exp, VM *vm)
{
  Val var = ListAt(vm->mem, exp, 1);
  Val val = Eval(ListAt(vm->mem, exp, 2), vm);
  Define(var, val, vm->env, vm->mem);
  return SymbolFor("ok");
}

Val EvalLet(Val exps, VM *vm)
{
  Val pairs = Tail(vm->mem, exps);
  Val val;
  while (!IsNil(pairs)) {
    Val var = Head(vm->mem, pairs);
    pairs = Tail(vm->mem, pairs);
    val = Eval(Head(vm->mem, pairs), vm);
    pairs = Tail(vm->mem, pairs);
    Define(var, val, vm->env, vm->mem);
  }
  return SymbolFor("ok");
}

Val EvalIf(Val exps, VM *vm)
{
  Val condition = ListAt(vm->mem, exps, 1);
  Val consequent = ListAt(vm->mem, exps, 2);
  Val alternative = ListAt(vm->mem, exps, 3);

  Val test = Eval(condition, vm);
  if (IsTrue(test)) {
    return Eval(consequent, vm);
  } else {
    return Eval(alternative, vm);
  }
}

Val EvalCond(Val exps, VM *vm)
{
  Val clauses = Tail(vm->mem, exps);
  while (!IsNil(clauses)) {
    Val clause = Head(vm->mem, clauses);
    Val test = Eval(Head(vm->mem, clause), vm);
    if (IsTrue(test)) {
      return Eval(Tail(vm->mem, clause), vm);
    }
    clauses = Tail(vm->mem, clauses);
  }

  return RuntimeError("Unmatched cond", clauses, vm->mem);
}

Val EvalAnd(Val exps, VM *vm)
{
  Val a = ListAt(vm->mem, exps, 1);
  Val b = ListAt(vm->mem, exps, 2);

  Val test = Eval(a, vm);
  if (!IsTrue(test)) {
    return BoolVal(false);
  }

  test = Eval(b, vm);
  return BoolVal(IsTrue(test));
}

Val EvalOr(Val exps, VM *vm)
{
  Val a = ListAt(vm->mem, exps, 1);
  Val b = ListAt(vm->mem, exps, 2);

  Val test = Eval(a, vm);
  if (IsTrue(test)) {
    return BoolVal(true);
  }

  test = Eval(b, vm);
  return BoolVal(IsTrue(test));
}

Val EvalImport(Val exps, VM *vm)
{
  Val file_arg = ListAt(vm->mem, exps, 1);
  Val name_arg = ListAt(vm->mem, exps, 2);

  char filename[BinaryLength(vm->mem, file_arg) + 1];
  BinaryToString(vm->mem, file_arg, filename);

  char *src = (char*)ReadFile(filename);
  if (!src) {
    return RuntimeError("Could not open", file_arg, vm->mem);
  }

  Val ast = Parse(src, StrLen(src), vm->mem);
  Val import_env = ExtendEnv(TopEnv(vm->mem, vm->env), vm->mem);

  Val old_env = vm->env;
  Val stmts = Tail(vm->mem, ast);
  while (!IsNil(stmts)) {
    Val stmt = Head(vm->mem, stmts);
    if (IsTagged(vm->mem, stmt, SymbolFor("let"))) {
      vm->env = import_env;
      EvalDefine(stmt, vm);
    }
    stmts = Tail(vm->mem, stmts);
  }
  vm->env = old_env;

  Val frame = Head(vm->mem, import_env);
  if (IsNil(name_arg)) {
    Val keys = DictKeys(vm->mem, frame);
    Val vals = DictValues(vm->mem, frame);
    for (u32 i = 0; i < DictSize(vm->mem, frame); i++) {
      Define(TupleAt(vm->mem, keys, i), TupleAt(vm->mem, vals, i), vm->env, vm->mem);
    }
  } else {
    Define(name_arg, frame, vm->env, vm->mem);
  }

  return SymbolFor("ok");
}

Val RuntimeError(char *message, Val exp, Mem *mem)
{
  Print(message);
  Print(": ");
  PrintVal(mem, exp);
  Print("\n");
  return SymbolFor("error");
}
